#include "FriendsController.hpp"
#include "helpers.hpp"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <iomanip>
#include <sstream>
#include <trantor/utils/Date.h>

using namespace drogon;

static std::pair<int, int> parseLimitOffset(const drogon::HttpRequestPtr &req) {
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
    return {limit, offset};
}

static void sendFollowResponse(const drogon::orm::Result &r, Callback callback) {
    Json::Value result(Json::arrayValue);
    for (const auto &row : r) {
        Json::Value item;
        item["id"] = row["id_uuid"].as<std::string>();
        item["login"] = row["login"].as<std::string>();

        std::string addedStr = row["added"].as<std::string>();
        std::replace(addedStr.begin(), addedStr.end(), ' ', 'T');
        addedStr += "Z";
        item["addedAt"] = addedStr;

        std::string imagePath = row["image"].as<std::string>();
        std::string imageBase64;
        if (!imagePath.empty()) {
            imageBase64 = loadImageAsBase64(imagePath);
        }
        if (!imageBase64.empty()) {
            item["image"] = imageBase64;
        } else {
            item["image"] = Json::nullValue;
        }
        result.append(item);
    }
    LOG_INFO << result.toStyledString();
    auto resp = HttpResponse::newHttpJsonResponse(result);
    resp->setStatusCode(k200OK);
    callback(resp);
}

static void fetchFollowList(int userId, const std::string &relationType, int limit, int offset,
                            Callback callback) {
    auto db = getDbClient();
    std::string sql;

    if (relationType == "following") {
        sql = R"sql(
            SELECT u.id_uuid, u.login, f.added, u.image
            FROM friends f
            JOIN users u ON u.id = f.id_friend
            WHERE f.id_user = $1
            ORDER BY f.added DESC
            LIMIT $2::integer OFFSET $3::integer
        )sql";
    } else if (relationType == "followers") {
        sql = R"sql(
            SELECT u.id_uuid, u.login, f.added, u.image
            FROM friends f
            JOIN users u ON u.id = f.id_user
            WHERE f.id_friend = $1
            ORDER BY f.added DESC
            LIMIT $2::integer OFFSET $3::integer
        )sql";
    } else {
        sendBadRequest("Invalid relation type", callback);
        return;
    }

    db->execSqlAsync(
        sql, [callback](const drogon::orm::Result &r) { sendFollowResponse(r, callback); },
        sendDbErrorResponse(callback), userId, std::to_string(limit), std::to_string(offset));
}

void FriendsController::addFriend(const HttpRequestPtr &req, Callback &&callback) {
    verifyToken(req, [callback, req](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }
        std::string currentLogin = *loginOpt;

        auto json = req->getJsonObject();
        if (!json || !(*json).isMember("id") || !(*json)["id"].isString()) {
            sendBadRequest("Missing or invalid 'id' (must be UUID string)", callback);
            return;
        }

        std::string friendId = (*json)["id"].asString();
        if (friendId.length() != 36) {
            sendBadRequest("Invalid friend's UUID format", callback);
            return;
        }

        auto db = getDbClient();

        db->execSqlAsync(
            "SELECT id_uuid FROM users WHERE login = $1",
            [callback, db, currentLogin, friendId](const drogon::orm::Result &r) {
                if (r.empty()) {
                    sendNotFound("User not found", callback);
                    return;
                }

                std::string userId = r[0]["id_uuid"].as<std::string>();

                if (userId == friendId) {
                    sendBadRequest("Cannot add yourself as a friend", callback);
                    return;
                }

                db->execSqlAsync(
                    "SELECT id FROM users WHERE id_uuid = $1",
                    [callback, db, userId, friendId](const drogon::orm::Result &r2) {
                        if (r2.empty()) {
                            sendNotFound("Friend not found", callback);
                            return;
                        }

                        db->execSqlAsync(
                            R"sql(
                                INSERT INTO friends (id_user, id_friend, added)
                                VALUES (
                                    (SELECT id FROM users WHERE id_uuid = $1::uuid),
                                    (SELECT id FROM users WHERE id_uuid = $2::uuid), 
                                    CURRENT_TIMESTAMP
                                )
                                ON CONFLICT (id_user, id_friend) DO UPDATE
                                SET added = EXCLUDED.added
                            )sql",
                            [callback](const drogon::orm::Result &) {
                                Json::Value ret;
                                ret["status"] = "ok";
                                auto resp = HttpResponse::newHttpJsonResponse(ret);
                                resp->setStatusCode(k200OK);
                                callback(resp);
                            },
                            sendDbErrorResponse(callback), userId, friendId);
                    },
                    sendDbErrorResponse(callback), friendId);
            },
            sendDbErrorResponse(callback), currentLogin);
    });
}

void FriendsController::removeFriend(const HttpRequestPtr &req, Callback &&callback) {
    verifyToken(req, [callback, req](std::optional<std::string> loginOpt) {
        LOG_INFO << "want to delete from friends";
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }
        std::string currentLogin = *loginOpt;

        auto json = req->getJsonObject();
        if (!json || !(*json).isMember("id") || !(*json)["id"].isString()) {
            sendBadRequest("Missing or invalid 'id' (must be UUID string)", callback);
            return;
        }

        std::string friendId = (*json)["id"].asString();
        if (friendId.length() != 36) {
            sendBadRequest("Invalid friend's UUID format", callback);
            return;
        }

        auto db = getDbClient();

        db->execSqlAsync(
            "SELECT id_uuid FROM users WHERE login = $1",
            [callback, db, currentLogin, friendId](const drogon::orm::Result &r) {
                if (r.empty()) {
                    sendNotFound("User not found", callback);
                    return;
                }
                std::string userId = r[0]["id_uuid"].as<std::string>();

                db->execSqlAsync(
                    "SELECT id FROM users WHERE id_uuid = $1",
                    [callback, db, userId, friendId](const drogon::orm::Result &r2) {
                        if (r2.empty()) {
                            Json::Value ret;
                            ret["status"] = "ok";
                            auto resp = HttpResponse::newHttpJsonResponse(ret);
                            resp->setStatusCode(k200OK);
                            callback(resp);
                            return;
                        }

                        db->execSqlAsync(
                            R"sql(DELETE FROM friends 
                            WHERE id_user = (SELECT id FROM users WHERE id_uuid = $1) 
                            AND id_friend = (SELECT id FROM users WHERE id_uuid = $2))sql",
                            [callback](const drogon::orm::Result &) {
                                Json::Value ret;
                                ret["status"] = "ok";
                                auto resp = HttpResponse::newHttpJsonResponse(ret);
                                resp->setStatusCode(k200OK);
                                callback(resp);
                            },
                            sendDbErrorResponse(callback), userId, friendId);
                    },
                    sendDbErrorResponse(callback), friendId);
            },
            sendDbErrorResponse(callback), currentLogin);
    });
}

void FriendsController::getFollowingList(const HttpRequestPtr &req, Callback &&callback,
                                         std::string targetUserId) {
    verifyToken(req, [callback, req, targetUserId](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }

        std::string currentLogin = *loginOpt;

        auto [lim, off] = parseLimitOffset(req);
        int limit = lim, offset = off;
        if (limit < 0 || offset < 0) {
            sendBadRequest("limit or offset is incorrect", callback);
            return;
        }

        bool isMeRequest = (targetUserId == "me");
        std::string uuidParam = isMeRequest ? "00000000-0000-0000-0000-000000000000" : targetUserId;

        auto db = getDbClient();
        db->execSqlAsync(
            "SELECT id FROM users WHERE ($3::boolean = true AND login = $2) OR ($3::boolean = "
            "false AND id_uuid = $1::uuid)",
            [callback, limit, offset, targetUserId](const drogon::orm::Result &r) {
                if (r.empty()) {
                    sendNotFound("User not found", callback);
                    return;
                }

                int targetIntId = r[0]["id"].as<int>();

                fetchFollowList(targetIntId, "following", limit, offset, callback);
            },
            sendDbErrorResponse(callback), uuidParam, currentLogin, isMeRequest);
    });
}

void FriendsController::getFollowersList(const HttpRequestPtr &req, Callback &&callback,
                                         std::string targetUserId) {
    verifyToken(req, [callback, req, targetUserId](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }

        std::string currentLogin = *loginOpt;

        auto [lim, off] = parseLimitOffset(req);
        int limit = lim, offset = off;
        if (limit < 0 || offset < 0) {
            sendBadRequest("limit or offset is incorrect", callback);
            return;
        }

        bool isMeRequest = (targetUserId == "me");
        std::string uuidParam = isMeRequest ? "00000000-0000-0000-0000-000000000000" : targetUserId;

        auto db = getDbClient();
        db->execSqlAsync(
            "SELECT id FROM users WHERE ($3::boolean = true AND login = $2) OR ($3::boolean = "
            "false AND id_uuid = $1::uuid)",
            [callback, limit, offset, targetUserId](const drogon::orm::Result &r) {
                if (r.empty()) {
                    sendNotFound("User not found", callback);
                    return;
                }

                int targetIntId = r[0]["id"].as<int>();

                fetchFollowList(targetIntId, "followers", limit, offset, callback);
            },
            sendDbErrorResponse(callback), uuidParam, currentLogin, isMeRequest);
    });
}

void FriendsController::getUser(const HttpRequestPtr &req, Callback &&callback, std::string login) {
    verifyToken(req, [callback, login](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }
        if (login.empty()) {
            sendBadRequest("Login cannot be empty", callback);
            return;
        }
        LOG_INFO << "need to find user";

        std::string currentLogin = *loginOpt;
        bool is_me = (login == currentLogin ? true : false);

        auto db = getDbClient();
        db->execSqlAsync(
            R"sql(
                    SELECT u.id_uuid, u.login, u.image,
                        EXISTS (
                            SELECT 1 FROM friends f
                            WHERE f.id_user = (SELECT id FROM users WHERE login = $2)
                                AND f.id_friend = u.id
                        ) AS is_followed
                    FROM users u
                    WHERE u.login = $1
                )sql",
            [callback, is_me](const drogon::orm::Result &r) {
                if (r.empty()) {
                    sendNotFound("User with this nickname does not exist", callback);
                    return;
                }
                auto row = r[0];
                Json::Value result;
                result["id"] = row["id_uuid"].as<std::string>();
                result["login"] = row["login"].as<std::string>();

                std::string imagePath = row["image"].as<std::string>();
                std::string imageBase64;
                if (!imagePath.empty()) {
                    imageBase64 = loadImageAsBase64(imagePath);
                }
                if (!imageBase64.empty()) {
                    result["image"] = imageBase64;
                } else {
                    result["image"] = Json::nullValue;
                }
                result["isFollowed"] = row["is_followed"].as<bool>();
                result["isMe"] = is_me;
                auto resp = HttpResponse::newHttpJsonResponse(result);
                resp->setStatusCode(k200OK);
                callback(resp);
            },
            sendDbErrorResponse(callback), login, currentLogin);
    });
}