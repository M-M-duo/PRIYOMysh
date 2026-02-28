#include <drogon/utils/Utilities.h>
#include "AuthController.h"
#include "helpers.h"

using namespace drogon;


void AuthController::ping(const HttpRequestPtr& req,
                              Callback&& callback) {
    Json::Value ret;
    ret["status"] = "ok";
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(k200OK);
    callback(resp);
}

void AuthController::registerUser(const HttpRequestPtr& req,
                                   Callback&& callback) {
    auto json = req->getJsonObject();
    if (!json) {
        LOG_INFO << "json in AuthController::registerUser does not exist";
        Json::Value ret; ret["reason"] = "Wrong profile data";
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    LOG_INFO << "json in AuthController::registerUser exists";
    

    auto login = (*json)["login"].asString();
    auto email = (*json)["email"].asString();
    auto password = (*json)["password"].asString();
    auto isPublic = (*json)["isPublic"].asBool();
    auto phone = (*json)["phone"].asString();
    auto image = json->get("image", "").asString();

    LOG_INFO << "got all values from json in AuthController::registerUser";


    if (!validateLogin(login)) {
        Json::Value ret;
        ret["reason"] = "Incorrect login format";
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    if (!validateEmail(email)) {
        Json::Value ret;
        ret["reason"] = "Incorrect email format";
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(k400BadRequest);
        callback(resp);         
        return;
    }

    if (!validatePasswordStrength(password)) {
        Json::Value ret;
        ret["reason"] = "Weak password";
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    if (!validatePhone(phone)) {
        Json::Value ret;
        ret["reason"] = "Incorrect phone format";
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    if (!validateImage(image)) {
        Json::Value ret;
        ret["reason"] = "Incorrect image format";
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }


    auto db = getDbClient();
    if (!db) {
        LOG_ERROR << "db client is null in AuthController::registerUser";
        return;
    }

    db->execSqlAsync("SELECT login, email, phone FROM users WHERE login=$1 OR email=$2 OR phone=$3",
        [callback, db, login, email, password, isPublic, phone, image](const drogon::orm::Result& r) {
            if (!r.empty()) {
                Json::Value ret; ret["reason"]="User with this login, email or phone already exists";
                auto resp = HttpResponse::newHttpJsonResponse(ret);
                resp->setStatusCode(k409Conflict);
                callback(resp);
                return;
            }

            std::string hashed = hashPassword(password);

            db->execSqlAsync("INSERT INTO users (login, email, password, is_public, phone, image) "
                                "VALUES ($1, $2, $3, $4, $5, $6) RETURNING *",
                [callback, login, email, isPublic, phone, image](const drogon::orm::Result& r) {
                    Json::Value profile;
                    profile["login"] = login;
                    profile["email"] = email;
                    profile["isPublic"] = isPublic;
                    if (!phone.empty()) profile["phone"] = phone;
                    if (!image.empty()) profile["image"] = image;

                    Json::Value ret;
                    ret["profile"] = profile;
                    auto resp = HttpResponse::newHttpJsonResponse(ret);
                    resp->setStatusCode(k201Created);
                    callback(resp);
                },
                [callback](const drogon::orm::DrogonDbException& e) {
                    LOG_ERROR << e.base().what();
                    Json::Value ret; ret["reason"] = "Wrong profile data";
                    auto resp = HttpResponse::newHttpJsonResponse(ret);
                    resp->setStatusCode(k400BadRequest);
                    callback(resp);
                },
                login, email, hashed, isPublic, phone, image);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << e.base().what();
            Json::Value ret; ret["reason"] = "Wrong profile data";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
        },
        login, email, phone);
}

void AuthController::signIn(const HttpRequestPtr& req,
                            Callback&& callback) {
    auto json = req->getJsonObject();

    if (!json) {
        LOG_INFO << "json in AuthController::signIn does not exist";
        Json::Value ret; ret["reason"] = "Wrong profile data";
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    LOG_INFO << "json in AuthController::signIn exists";

    auto login = (*json)["login"].asString();
    auto password = (*json)["password"].asString();

    auto db = getDbClient();
    if (!db) {
        LOG_ERROR << "db client is null in AuthController::registerUser";
        return;
    }

    db->execSqlAsync("SELECT password, token_number, update_token FROM users WHERE login = $1",
        [callback, login, password](const drogon::orm::Result& r) {
            if (r.empty()) {
                Json::Value ret; ret["reason"] = "User with this login and password was not found";
                auto resp = HttpResponse::newHttpJsonResponse(ret);
                resp->setStatusCode(k401Unauthorized);
                callback(resp);
                return;
            }
            auto row = r[0];
            std::string hashed = row["password"].as<std::string>();
            if (!checkPassword(password, hashed)) {
                Json::Value ret; ret["reason"] = "User with this login and password was not found";
                auto resp = HttpResponse::newHttpJsonResponse(ret);
                resp->setStatusCode(k401Unauthorized);
                callback(resp);
                return;
            }

            int token_number = row["token_number"].as<int>();
            int update_token = row["update_token"].as<int>();

            auto db = getDbClient();
            db->execSqlAsync("UPDATE users SET update_token = update_token + 1 WHERE login = $1 RETURNING update_token",
                [callback, login, token_number](const drogon::orm::Result& r) {
                    int new_update_token = r[0]["update_token"].as<int>();
                    std::string jwt = createToken(login, token_number, new_update_token);
                    Json::Value ret;
                    ret["token"] = jwt;
                    auto resp = HttpResponse::newHttpJsonResponse(ret);
                    resp->setStatusCode(k200OK);
                    callback(resp);
                },
                [callback](const drogon::orm::DrogonDbException& e) {
                    LOG_ERROR << e.base().what();
                    Json::Value ret; ret["reason"] = "User with this login and password was not found";
                    auto resp = HttpResponse::newHttpJsonResponse(ret);
                    resp->setStatusCode(k401Unauthorized);
                    callback(resp);
                },
                login);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << e.base().what();
            Json::Value ret; ret["reason"] = "User with this login and password was not found";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k401Unauthorized);
            callback(resp);
        },
        login);
}