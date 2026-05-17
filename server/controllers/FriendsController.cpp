#include "FriendsController.hpp"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <trantor/utils/Date.h>
#include <iomanip>
#include <sstream>
#include "helpers.hpp"

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

static void
sendFollowResponse(const drogon::orm::Result &r, Callback callback) {
    Json::Value result(Json::arrayValue);
    for (const auto &row : r) {
        Json::Value item;
        item["login"] = row["login"].as<std::string>();
        std::string addedStr = row["added"].as<std::string>();
        std::replace(addedStr.begin(), addedStr.end(), ' ', 'T');
        addedStr += "Z";
        item["addedAt"] = addedStr;
        std::string image = row["image"].as<std::string>();
        if (!image.empty()) {
            item["image"] = image;
        } else {
            item["image"] = Json::nullValue;
        }
        result.append(item);
    }
    auto resp = HttpResponse::newHttpJsonResponse(result);
    resp->setStatusCode(k200OK);
    callback(resp);
}

static void fetchFollowList(
    int userId,
    const std::string &relationType,
    int limit,
    int offset,
    Callback callback
) {
    auto db = getDbClient();
    std::string sql;
    if (relationType == "following") {
        sql = R"sql(
            SELECT u.login, f.added, u.image
            FROM friends f
            JOIN users u ON u.id = f.id_friend
            WHERE f.id_user = $1
            ORDER BY f.added DESC
            LIMIT $2::integer OFFSET $3::integer
        )sql";
    } else if (relationType == "followers") {
        sql = R"sql(
            SELECT u.login, f.added, u.image
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
        sql,
        [callback](const drogon::orm::Result &r) {
            sendFollowResponse(r, callback);
        },
        sendDbErrorResponse(callback), userId, std::to_string(limit),
        std::to_string(offset)
    );
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

void FriendsController::getFollowingList(
    const HttpRequestPtr &req,
    Callback &&callback,
    std::string login
) {
    verifyToken(
        req,
        [callback, req, login](std::optional<std::string> loginOpt) {
            if (!loginOpt) {
                sendUnauthorized(callback);
                return;
            }

            auto [lim, off] = parseLimitOffset(req);
            int limit = lim, offset = off;
            if (limit < 0 || offset < 0) {
                sendBadRequest("limit or offset is incorrect", callback);
                return;
            }

            auto db = getDbClient();
            db->execSqlAsync(
                "SELECT id FROM users WHERE login = $1",
                [callback, limit, offset](const drogon::orm::Result &r) {
                    if (r.empty()) {
                        sendNotFound("User not found", callback);
                        return;
                    }
                    int userId = r[0]["id"].as<int>();
                    fetchFollowList(
                        userId, "following", limit, offset, callback
                    );
                },
                sendDbErrorResponse(callback), login
            );
        }
    );
}

void FriendsController::getFollowersList(
    const HttpRequestPtr &req,
    Callback &&callback,
    std::string login
) {
    verifyToken(
        req,
        [callback, req, login](std::optional<std::string> loginOpt) {
            if (!loginOpt) {
                sendUnauthorized(callback);
                return;
            }

            auto [lim, off] = parseLimitOffset(req);
            int limit = lim, offset = off;
            if (limit < 0 || offset < 0) {
                sendBadRequest("limit or offset is incorrect", callback);
                return;
            }

            auto db = getDbClient();
            db->execSqlAsync(
                "SELECT id FROM users WHERE login = $1",
                [callback, limit, offset](const drogon::orm::Result &r) {
                    if (r.empty()) {
                        sendNotFound("User not found", callback);
                        return;
                    }
                    int userId = r[0]["id"].as<int>();
                    fetchFollowList(
                        userId, "followers", limit, offset, callback
                    );
                },
                sendDbErrorResponse(callback), login
            );
        }
    );
}

void FriendsController::getUser(
    const HttpRequestPtr &req,
    Callback &&callback,
    std::string login
) {
    verifyToken(req, [callback, login](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }
        if (login.empty()) {
            sendBadRequest("Login cannot be empty", callback);
            return;
        }

        std::string currentLogin = *loginOpt;

        auto db = getDbClient();
        db->execSqlAsync(
            R"sql(
                    SELECT u.login, u.image,
                        EXISTS (
                            SELECT 1 FROM friends f
                            WHERE f.id_user = (SELECT id FROM users WHERE login = $2)
                                AND f.id_friend = u.id
                        ) AS is_followed
                    FROM users u
                    WHERE u.login = $1
                )sql",
            [callback](const drogon::orm::Result &r) {
                if (r.empty()) {
                    sendNotFound(
                        "User with this nickname does not exist", callback
                    );
                    return;
                }
                auto row = r[0];
                Json::Value result;
                result["login"] = row["login"].as<std::string>();
                std::string image = row["image"].as<std::string>();
                if (!image.empty()) {
                    result["image"] = image;
                } else {
                    result["image"] = Json::nullValue;
                }
                result["isFollowed"] = row["is_followed"].as<bool>();
                auto resp = HttpResponse::newHttpJsonResponse(result);
                resp->setStatusCode(k200OK);
                callback(resp);
            },
            sendDbErrorResponse(callback), login, currentLogin
        );
    });
}