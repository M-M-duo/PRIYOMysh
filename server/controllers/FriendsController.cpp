#include "FriendsController.hpp"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <trantor/utils/Date.h>
#include <iomanip>
#include <sstream>
#include "helpers.hpp"

using namespace drogon;

static auto sendDbErrorResponse(Callback callback) {
    return [callback](const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << e.base().what();
        Json::Value ret;
        ret["reason"] = "Internal error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    };
}

static void sendUnauthorized(Callback callback) {
    Json::Value ret;
    ret["reason"] = "Token is incorrect";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(k401Unauthorized);
    callback(resp);
}

static void sendBadRequest(const std::string &reason, Callback callback) {
    Json::Value ret;
    ret["reason"] = reason;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(k400BadRequest);
    callback(resp);
}

static void sendNotFound(const std::string &reason, Callback callback) {
    Json::Value ret;
    ret["reason"] = reason;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(k404NotFound);
    callback(resp);
}

static void sendInternalError(Callback callback) {
    Json::Value ret;
    ret["reason"] = "Internal error";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
}

void FriendsController::addFriend(
    const HttpRequestPtr &req,
    Callback &&callback
) {
    verifyToken(req, [callback, req](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }
        std::string currentLogin = *loginOpt;

        auto json = req->getJsonObject();
        if (!json || !(*json).isMember("login")) {
            sendBadRequest("Missing 'login' field", callback);
            return;
        }
        std::string friendLogin = (*json)["login"].asString();
        if (friendLogin.empty()) {
            sendBadRequest("Friend login cannot be empty", callback);
            return;
        }
        if (currentLogin == friendLogin) {
            sendBadRequest("Cannot add yourself as a friend", callback);
            return;
        }

        auto db = getDbClient();

        db->execSqlAsync(
            "SELECT id FROM users WHERE login = $1",
            [callback, db, currentLogin,
             friendLogin](const drogon::orm::Result &r) {
                if (r.empty()) {
                    sendNotFound("User not found", callback);
                    return;
                }
                int userId = r[0]["id"].as<int>();

                db->execSqlAsync(
                    "SELECT id FROM users WHERE login = $1",
                    [callback, db, userId,
                     friendLogin](const drogon::orm::Result &r2) {
                        if (r2.empty()) {
                            sendNotFound("Friend not found", callback);
                            return;
                        }
                        int friendId = r2[0]["id"].as<int>();

                        db->execSqlAsync(
                            R"sql(
                                INSERT INTO friends (id_user, id_friend, added)
                                VALUES ($1, $2, CURRENT_TIMESTAMP)
                                ON CONFLICT (id_user, id_friend) DO UPDATE
                                SET added = EXCLUDED.added
                            )sql",
                            [callback](const drogon::orm::Result &) {
                                Json::Value ret;
                                ret["status"] = "ok";
                                auto resp =
                                    HttpResponse::newHttpJsonResponse(ret);
                                resp->setStatusCode(k200OK);
                                callback(resp);
                            },
                            sendDbErrorResponse(callback), userId, friendId
                        );
                    },
                    sendDbErrorResponse(callback), friendLogin
                );
            },
            sendDbErrorResponse(callback), currentLogin
        );
    });
}

void FriendsController::removeFriend(
    const HttpRequestPtr &req,
    Callback &&callback
) {
    verifyToken(req, [callback, req](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }
        std::string currentLogin = *loginOpt;

        auto json = req->getJsonObject();
        if (!json || !(*json).isMember("login")) {
            sendBadRequest("Missing 'login' field", callback);
            return;
        }
        std::string friendLogin = (*json)["login"].asString();
        if (friendLogin.empty()) {
            sendBadRequest("Friend login cannot be empty", callback);
            return;
        }

        auto db = getDbClient();

        db->execSqlAsync(
            "SELECT id FROM users WHERE login = $1",
            [callback, db, currentLogin,
             friendLogin](const drogon::orm::Result &r) {
                if (r.empty()) {
                    sendNotFound("User not found", callback);
                    return;
                }
                int userId = r[0]["id"].as<int>();

                db->execSqlAsync(
                    "SELECT id FROM users WHERE login = $1",
                    [callback, db, userId,
                     friendLogin](const drogon::orm::Result &r2) {
                        if (r2.empty()) {
                            Json::Value ret;
                            ret["status"] = "ok";
                            auto resp = HttpResponse::newHttpJsonResponse(ret);
                            resp->setStatusCode(k200OK);
                            callback(resp);
                            return;
                        }
                        int friendId = r2[0]["id"].as<int>();

                        db->execSqlAsync(
                            "DELETE FROM friends WHERE id_user = $1 AND "
                            "id_friend = $2",
                            [callback](const drogon::orm::Result &) {
                                Json::Value ret;
                                ret["status"] = "ok";
                                auto resp =
                                    HttpResponse::newHttpJsonResponse(ret);
                                resp->setStatusCode(k200OK);
                                callback(resp);
                            },
                            sendDbErrorResponse(callback), userId, friendId
                        );
                    },
                    sendDbErrorResponse(callback), friendLogin
                );
            },
            sendDbErrorResponse(callback), currentLogin
        );
    });
}

void FriendsController::getFriendsList(
    const HttpRequestPtr &req,
    Callback &&callback
) {
    verifyToken(req, [callback, req](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }
        std::string currentLogin = *loginOpt;

        int limit = 5;
        auto limitParam = req->getParameter("limit");
        if (!limitParam.empty()) {
            limit = std::stoi(limitParam);
        }
        int offset = 0;
        auto offsetParam = req->getParameter("offset");
        if (!offsetParam.empty()) {
            offset = std::stoi(offsetParam);
        }
        if (limit < 0 || offset < 0) {
            sendBadRequest("limit or offset is incorrect", callback);
            return;
        }

        auto db = getDbClient();
        db->execSqlAsync(
            R"sql(
                SELECT u.login, f.added
                FROM friends f
                JOIN users u ON u.id = f.id_friend
                WHERE f.id_user = (SELECT id FROM users WHERE login = $1)
                ORDER BY f.added DESC
                LIMIT $2::integer OFFSET $3::integer
            )sql",
            [callback](const drogon::orm::Result &r) {
                Json::Value friends(Json::arrayValue);
                for (const auto &row : r) {
                    Json::Value fr;
                    fr["login"] = row["login"].as<std::string>();
                    // std::string addedStr = row["added"].as<std::string>();
                    // std::replace(addedStr.begin(), addedStr.end(), ' ', 'T');
                    // addedStr += "Z";
                    // fr["addedAt"] = addedStr;
                    fr["addedAt"] = row["added"].as<std::string>();
                    friends.append(fr);
                }
                auto resp = HttpResponse::newHttpJsonResponse(friends);
                resp->setStatusCode(k200OK);
                callback(resp);
            },
            sendDbErrorResponse(callback), currentLogin, std::to_string(limit),
            std::to_string(offset)
        );
    });
}