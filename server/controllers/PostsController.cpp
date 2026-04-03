#include "PostsController.h"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <trantor/utils/Date.h>
#include <iomanip>
#include <sstream>
#include "helpers.h"

using namespace drogon;

static void verifyToken(
    const HttpRequestPtr &req,
    std::function<void(std::optional<std::string>)> callback
) {
    auto auth = req->getHeader("Authorization");
    if (auth.empty() || auth.substr(0, 7) != "Bearer ") {
        std::cout << "nonBearer";
        callback(std::nullopt);
        return;
    }
    std::string token = auth.substr(7);
    auto payload = getTokenContent(token);
    if (!payload || payload->exp < std::chrono::system_clock::time_point()) {
        std::cout << "time stuff";
        callback(std::nullopt);
        return;
    }
    auto db = getDbClient();
    db->execSqlAsync(
        R"sql(SELECT token_number FROM users WHERE login = $1)sql",
        [payload, callback](const drogon::orm::Result &r) {
            if (r.empty()) {
                std::cout << "no such users";
                callback(std::nullopt);
                return;
            }
            int dbTokenNumber = r[0]["token_number"].as<int>();
            if (dbTokenNumber != payload->token_number) {
                std::cout << "wrong token_number";
                callback(std::nullopt);
                return;
            }
            callback(payload->login);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << e.base().what();
            callback(std::nullopt);
        },
        payload->login
    );
}

static void fetchPost(
    const std::string &postId,
    const std::string &currentLogin,
    std::function<void(const Json::Value &, int)> callback
) {
    auto db = getDbClient();
    db->execSqlAsync(
        R"sql(SELECT p.*, u.is_public as author_public, 
                     (SELECT string_agg(tag, ',') FROM tags WHERE id_post = p.id) as tags1 
                     FROM posts p JOIN users u ON u.login = p.author WHERE p.id_uuid = $1)sql",
        [callback, currentLogin, db](const drogon::orm::Result &r) {
            if (r.empty()) {
                callback(Json::Value(), 404);
                return;
            }
            auto row = r[0];
            std::string author = row["author"].as<std::string>();
            bool authorPublic = row["author_public"].as<bool>();
            std::string tagsStr = row["tags1"].as<std::string>();
            std::vector<std::string> tags;
            if (!tagsStr.empty()) {
                std::istringstream iss(tagsStr);
                std::string tag;
                while (std::getline(iss, tag, ',')) {
                    tags.push_back(tag);
                }
            }

            if (author != currentLogin && !authorPublic) {
                // здесь позже добавится проверка на то, является ли
                // пользователь другом
                callback(Json::Value(), 404);
                return;
            }

            Json::Value post;
            post["id"] = row["id_uuid"].as<std::string>();
            post["content"] = row["content"].as<std::string>();
            post["author"] = author;
            for (const auto &t : tags) {
                post["tags"].append(t);
            }
            post["createdAt"] = row["created_at"].as<std::string>();
            post["likesCount"] = 0;
            post["dislikesCount"] = 0;
            callback(post, 200);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << e.base().what();
            callback(Json::Value(), 500);
        },
        postId
    );
}

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

static Json::Value buildPostsJson(const drogon::orm::Result &r) {
    Json::Value posts(Json::arrayValue);
    for (const auto &row : r) {
        Json::Value post;
        post["id"] = row["id_uuid"].as<std::string>();
        post["content"] = row["content"].as<std::string>();
        post["author"] = row["author"].as<std::string>();

        std::string tagsStr = row["tags1"].as<std::string>();
        if (!tagsStr.empty()) {
            std::istringstream iss(tagsStr);
            std::string tag;
            while (std::getline(iss, tag, ',')) {
                post["tags"].append(tag);
            }
        }

        post["createdAt"] = row["created_at"].as<std::string>();
        // здесь потом добавлю подсчет лайков и дизлайков
        post["likesCount"] = 0;
        post["dislikesCount"] = 0;
        posts.append(post);
    }
    return posts;
}

static auto sendPostsResponse(Callback callback) {
    return [callback](const drogon::orm::Result &r) {
        auto resp =
            drogon::HttpResponse::newHttpJsonResponse(buildPostsJson(r));
        resp->setStatusCode(k200OK);
        callback(resp);
    };
}

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

static void sendForbidden(const std::string &reason, Callback callback) {
    Json::Value ret;
    ret["reason"] = reason;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(k403Forbidden);
    callback(resp);
}

static void sendInternalError(Callback callback) {
    Json::Value ret;
    ret["reason"] = "Internal error";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(k500InternalServerError);
    callback(resp);
}

void PostsController::newPost(const HttpRequestPtr &req, Callback &&callback) {
    verifyToken(req, [callback, req](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            Json::Value ret;
            ret["reason"] = "Token is incorrect";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k401Unauthorized);
            callback(resp);
            return;
        }
        std::string login = *loginOpt;

        auto json = req->getJsonObject();
        if (!json) {
            Json::Value ret;
            ret["reason"] = "Tags or content are incorrect";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        auto content = (*json)["content"].asString();
        auto tags = (*json)["tags"];
        if (!tags.isArray() || content.empty() || content.size() > 1000) {
            Json::Value ret;
            ret["reason"] = "Tags or content are incorrect";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        if (tags.size() > 20) {
            Json::Value ret;
            ret["reason"] = "Too many tags";
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        for (const auto &tag : tags) {
            if (!tag.isString() || tag.asString().size() > 20) {
                Json::Value ret;
                ret["reason"] = "Tag too long or invalid";
                auto resp = HttpResponse::newHttpJsonResponse(ret);
                resp->setStatusCode(k400BadRequest);
                callback(resp);
                return;
            }
        }

        auto db = getDbClient();
        auto now = trantor::Date::now();
        std::string createdAt =
            now.toCustomFormattedString("%Y-%m-%dT%H:%M:%S", true);

        db->execSqlAsync(
            R"sql(INSERT INTO posts (content, author, created_at) VALUES ($1, $2, 
            $3) RETURNING id, id_uuid)sql",
            [callback, tags, createdAt, login,
             content](const drogon::orm::Result &r) {
                if (r.empty()) {
                    Json::Value ret;
                    ret["reason"] = "Post creation failed";
                    auto resp = HttpResponse::newHttpJsonResponse(ret);
                    resp->setStatusCode(k500InternalServerError);
                    callback(resp);
                    return;
                }
                int postId = r[0]["id"].as<int>();
                std::string uuid = r[0]["id_uuid"].as<std::string>();

                auto db2 = getDbClient();
                for (const auto &tag : tags) {
                    db2->execSqlAsync(
                        R"sql(INSERT INTO tags (id_post, tag) VALUES ($1, $2))sql",
                        [](const drogon::orm::Result &) {},
                        [](const drogon::orm::DrogonDbException &e) {
                            LOG_ERROR << e.base().what();
                        },
                        postId, tag.asString()
                    );
                }

                Json::Value post;
                post["id"] = uuid;
                post["content"] = content;
                post["author"] = login;
                for (const auto &tag : tags) {
                    post["tags"].append(tag.asString());
                }
                post["createdAt"] = createdAt;
                post["likesCount"] = 0;
                post["dislikesCount"] = 0;

                auto resp = HttpResponse::newHttpJsonResponse(post);
                resp->setStatusCode(k200OK);
                callback(resp);
            },
            [callback](const drogon::orm::DrogonDbException &e) {
                LOG_ERROR << e.base().what();
                Json::Value ret;
                ret["reason"] = "Post creation failed";

                auto resp = HttpResponse::newHttpJsonResponse(ret);
                resp->setStatusCode(k500InternalServerError);
                callback(resp);
            },
            content, login, createdAt
        );
    });
}

void PostsController::getPost(
    const HttpRequestPtr &req,
    Callback &&callback,
    std::string postId
) {
    verifyToken(
        req,
        [callback, req, postId](std::optional<std::string> loginOpt) {
            if (!loginOpt) {
                sendUnauthorized(callback);
                return;
            }
            std::string currentLogin = *loginOpt;

            fetchPost(
                postId, currentLogin,
                [callback, postId](const Json::Value &post, int status) {
                    if (status == 404) {
                        sendNotFound("The post is not found", callback);
                        return;
                    }
                    if (status != 200) {
                        sendInternalError(callback);
                        return;
                    }
                    // здесь потом добавить подсчет лайков
                    auto resp = HttpResponse::newHttpJsonResponse(post);
                    resp->setStatusCode(k200OK);
                    callback(resp);
                }
            );
        }
    );
}

void PostsController::myFeed(const HttpRequestPtr &req, Callback &&callback) {
    verifyToken(req, [callback, req](std::optional<std::string> loginOpt) {
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

        auto db = getDbClient();
        db->execSqlAsync(
            R"sql(
                SELECT p.id_uuid, p.content, p.author, p.created_at,
                       (SELECT string_agg(tag, ',') FROM tags WHERE id_post = p.id) as tags1
                FROM posts p
                WHERE p.author = $1
                ORDER BY p.created_at DESC
                LIMIT $2 OFFSET $3
            )sql",
            sendPostsResponse(callback), sendDbErrorResponse(callback),
            currentLogin, std::to_string(limit), std::to_string(offset)
        );
    });
}

void PostsController::userFeed(
    const HttpRequestPtr &req,
    Callback &&callback,
    std::string login
) {
    verifyToken(
        req,
        [callback, req, login](std::optional<std::string> currentLoginOpt) {
            if (!currentLoginOpt) {
                sendUnauthorized(callback);
                return;
            }
            std::string currentLogin = *currentLoginOpt;
            auto [lim, off] = parseLimitOffset(req);
            int limit = lim, offset = off;

            if (limit < 0 || offset < 0) {
                sendBadRequest("limit or offset is incorrect", callback);
                return;
            }

            auto db = getDbClient();
            db->execSqlAsync(
                R"sql(SELECT is_public FROM users WHERE login = $1)sql",
                [callback, db, currentLogin, login, limit,
                 offset](const drogon::orm::Result &r) {
                    if (r.empty()) {
                        sendNotFound("User not found", callback);
                        return;
                    }
                    bool isPublic = r[0]["is_public"].as<bool>();
                    if (currentLogin != login && !isPublic) {
                        // здесь потом добавить проверку на друзей
                        sendForbidden(
                            "You are not allowed to see this profile", callback
                        );
                        return;
                    }
                    db->execSqlAsync(
                        R"sql(
                        SELECT p.id_uuid, p.content, p.author, p.created_at,
                               (SELECT string_agg(tag, ',') FROM tags WHERE id_post = p.id) as tags1
                        FROM posts p
                        WHERE p.author = $1
                        ORDER BY p.created_at DESC
                        LIMIT $2::integer OFFSET $3::integer
                    )sql",
                        sendPostsResponse(callback),
                        sendDbErrorResponse(callback), login,
                        std::to_string(limit), std::to_string(offset)
                    );
                },
                sendDbErrorResponse(callback), login
            );
        }
    );
}

void PostsController::newsFeed(const HttpRequestPtr &req, Callback &&callback) {
    verifyToken(req, [callback, req](std::optional<std::string> loginOpt) {
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

        // потом здесь надо сделать проверку на друзей, пока что лента состоит
        // только из постов пользователей с публичным аккаунтом
        auto db = getDbClient();
        db->execSqlAsync(
            R"sql(
                SELECT p.id_uuid, p.content, p.author, p.created_at,
                       (SELECT string_agg(tag, ',') FROM tags WHERE id_post = p.id) as tags1
                FROM posts p
                JOIN users u ON u.login = p.author
                WHERE u.is_public = true
                ORDER BY p.created_at DESC
                LIMIT $1 OFFSET $2
            )sql",
            sendPostsResponse(callback), sendDbErrorResponse(callback),
            std::to_string(limit), std::to_string(offset)
        );
    });
}