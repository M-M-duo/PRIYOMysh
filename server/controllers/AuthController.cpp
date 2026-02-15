#include "AuthController.h"
#include "helpers.h"
#include <drogon/utils/Utilities.h>

using namespace drogon;

void AuthController::registerUser(const HttpRequestPtr& req,
                                   Callback&& callback) {
    auto json = req->getJsonObject();
    if (!json) {
        Json::Value ret; 
        ret["reason"] = "Wrong profile data";
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto login = (*json)["login"].asString();
    auto email = (*json)["email"].asString();
    auto password = (*json)["password"].asString();
    auto isPublic = (*json)["isPublic"].asBool();
    auto phone = json->get("phone", "").asString();
    auto image = json->get("image", "").asString();

    auto db = getDbClient();

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
