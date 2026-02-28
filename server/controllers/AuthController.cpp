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
        LOG_INFO << "json in AuthController does not exist";
        Json::Value ret; ret["reason"] = "Wrong profile data";
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }
    LOG_INFO << "json in AuthController exists";
    

    auto login = (*json)["login"].asString();
    auto email = (*json)["email"].asString();
    auto password = (*json)["password"].asString();
    auto isPublic = (*json)["isPublic"].asBool();
    auto phone = (*json)["phone"].asString();
    auto image = json->get("image", "").asString();

    LOG_INFO << "got all values from json in AuthController";


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
        LOG_ERROR << "db client is null in registerUser";
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