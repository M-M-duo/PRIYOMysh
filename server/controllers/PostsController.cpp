#include "PostsController.h"
#include "helpers.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpTypes.h>
#include <trantor/utils/Date.h>
#include <sstream>
#include <iomanip>

using namespace drogon;

static void verifyToken(const HttpRequestPtr& req,
                        std::function<void(std::optional<std::string>)> callback) {
    auto auth = req->getHeader("Authorization");
    if (auth.empty() || auth.substr(0, 7) != "Bearer ") {
        callback(std::nullopt);
        return;
    }
    std::string token = auth.substr(7);
    auto payload = getTokenContent(token);
    if (!payload || payload->exp < time(nullptr)) {
        callback(std::nullopt);
        return;
    }
    auto db = getDbClient();
    db->execSqlAsync("SELECT token_number FROM users WHERE login = $1",
        [payload, callback](const drogon::orm::Result& r) {
            if (r.empty()) {
                callback(std::nullopt);
                return;
            }
            int dbTokenNumber = r[0]["token_number"].as<int>();
            if (dbTokenNumber != payload->token_number) {
                callback(std::nullopt);
                return;
            }
            callback(payload->login);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << e.base().what();
            callback(std::nullopt);
        },
        payload->login);
}

void PostsController::newPost(const HttpRequestPtr& req, Callback&& callback) {
    verifyToken(req, [callback, req](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            Json::Value ret; ret["reason"] = "Token is incorrect";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k401Unauthorized);
            callback(resp);
            return;
        }
        std::string login = *loginOpt;

        auto json = req->getJsonObject();
        if (!json) {
            Json::Value ret; ret["reason"] = "Tags or content are incorrect";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        auto content = (*json)["content"].asString();
        auto tags = (*json)["tags"];
        if (!tags.isArray() || content.empty() || content.size() > 1000) {
            Json::Value ret; ret["reason"] = "Tags or content are incorrect";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        if (tags.size() > 20) {
            Json::Value ret; ret["reason"] = "Too many tags";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        for (const auto& tag : tags) {
            if (!tag.isString() || tag.asString().size() > 20) {
                Json::Value ret; ret["reason"] = "Tag too long or invalid";
                auto resp = HttpResponse::newHttpJsonResponse(ret);
                resp->setStatusCode(k400BadRequest);
                callback(resp);
                return;
            }
        }

        auto db = getDbClient();
        auto now = trantor::Date::now();
        std::string createdAt = now.toCustomFormattedString("%Y-%m-%dT%H:%M:%SZ", true);

        db->execSqlAsync("INSERT INTO posts (content, author, created_at) VALUES ($1, $2, $3) RETURNING id, id_uuid",
            [callback, tags, createdAt, login, content](const drogon::orm::Result& r) {
                if (r.empty()) {
                    Json::Value ret; ret["reason"] = "Post creation failed";
                    auto resp = HttpResponse::newHttpJsonResponse(ret);
                    resp->setStatusCode(k500InternalServerError);
                    callback(resp);
                    return;
                }
                int postId = r[0]["id"].as<int>();
                std::string uuid = r[0]["id_uuid"].as<std::string>();

                auto db2 = getDbClient();
                for (const auto& tag : tags) {
                    db2->execSqlAsync("INSERT INTO tags (id_post, tag) VALUES ($1, $2)",
                        [](const drogon::orm::Result&) {},
                        [](const drogon::orm::DrogonDbException& e) { LOG_ERROR << e.base().what(); },
                        postId, tag.asString());
                }

                Json::Value post;
                post["id"] = uuid;
                post["content"] = content;
                post["author"] = login;
                for (const auto& tag : tags) post["tags"].append(tag.asString());
                post["createdAt"] = createdAt;
                post["likesCount"] = 0;
                post["dislikesCount"] = 0;

                auto resp = HttpResponse::newHttpJsonResponse(post);
                resp->setStatusCode(k200OK);
                callback(resp);
            },
            [callback](const drogon::orm::DrogonDbException& e) {
                LOG_ERROR << e.base().what();
                Json::Value ret; ret["reason"] = "Post creation failed";
                
                auto resp = HttpResponse::newHttpJsonResponse(ret);
                resp->setStatusCode(k500InternalServerError);
                callback(resp);
            },
            content, login, createdAt);
    });
}