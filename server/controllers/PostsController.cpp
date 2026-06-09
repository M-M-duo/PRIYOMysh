#include "PostsController.hpp"
#include "helpers.hpp"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <iomanip>
#include <sstream>
#include <trantor/utils/Date.h>

using namespace drogon;

static void fetchPost(const std::string &postId, const std::string &currentLogin,
                      std::function<void(const Json::Value &, int)> callback) {
    auto db = getDbClient();
    db->execSqlAsync(
        R"sql(SELECT p.*, u.login as author, u.id_uuid as author_id, u.is_public as author_public, 
                     (SELECT string_agg(tag, ',') FROM tags WHERE id_post = p.id) as tags1,
                     (SELECT string_agg(img, ',') FROM media WHERE id_post = p.id) as images,
                     (SELECT COUNT(*) FROM likes WHERE id_post = p.id AND is_like = true) as likesCount,
                     (SELECT COUNT(*) FROM likes WHERE id_post = p.id AND is_like = false) as dislikesCount
                     FROM posts p JOIN users u ON u.id = p.author_id WHERE p.id_uuid = $1)sql",
        [callback, currentLogin, db](const drogon::orm::Result &r) {
            if (r.empty()) {
                callback(Json::Value(), 404);
                return;
            }
            auto row = r[0];
            std::string author = row["author"].as<std::string>();
            std::string author_id = row["author_id"].as<std::string>();
            bool authorPublic = row["author_public"].as<bool>();
            std::string imagesStr = row["images"].as<std::string>();
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
                db->execSqlAsync(
                    R"sql(SELECT id FROM friends WHERE id_user = (SELECT id FROM users WHERE login = $1) 
                                 AND id_friend = (SELECT id FROM users WHERE login = $2)sql",
                    [callback, row, tags, author](const drogon::orm::Result &r) {
                        if (r.empty()) {
                            callback(Json::Value(), 404);
                            return;
                        }
                    },
                    [callback](const drogon::orm::DrogonDbException &e) {
                        LOG_ERROR << e.base().what();
                        callback(Json::Value(), 500);
                        return;
                    },
                    author, currentLogin);
            }

            Json::Value post;
            post["id"] = row["id_uuid"].as<std::string>();
            post["content"] = row["content"].as<std::string>();
            post["author"] = author;
            post["author_id"] = author_id;
            for (const auto &t : tags) {
                post["tags"].append(t);
            }
            if (!imagesStr.empty()) {
                std::istringstream iss(imagesStr);
                std::string imgPath;
                while (std::getline(iss, imgPath, ',')) {
                    std::string base64 = loadImageAsBase64(imgPath);
                    if (!base64.empty()) {
                        post["img"].append(base64);
                    }
                }
            }
            post["createdAt"] = row["created_at"].as<std::string>();
            post["likesCount"] = row["likesCount"].as<int>();
            post["dislikesCount"] = row["dislikesCount"].as<int>();
            callback(post, 200);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << e.base().what();
            callback(Json::Value(), 500);
        },
        postId);
}

static int parseLimit(const drogon::HttpRequestPtr &req) {
    int limit = 5;
    auto limitParam = req->getParameter("limit");
    if (!limitParam.empty()) {
        limit = std::stoi(limitParam);
    }
    return limit;
}

static Json::Value buildPostsJson(const drogon::orm::Result &r) {
    Json::Value posts(Json::arrayValue);
    for (const auto &row : r) {
        std::string imagePath = row["author_avatar"].as<std::string>();
        std::string imageBase64;
        if (!imagePath.empty()) {
            imageBase64 = loadImageAsBase64(imagePath);
        }

        Json::Value post;
        post["id"] = row["id_uuid"].as<std::string>();
        post["content"] = row["content"].as<std::string>();
        post["author"] = row["author"].as<std::string>();
        post["author_id"] = row["author_id"].as<std::string>();
        if (!imageBase64.empty()) {
            post["author_avatar"] = imageBase64;
        } else {
            post["author_avatar"] = Json::nullValue;
        }

        std::string tagsStr = row["tags1"].as<std::string>();
        if (!tagsStr.empty()) {
            std::istringstream iss(tagsStr);
            std::string tag;
            while (std::getline(iss, tag, ',')) {
                post["tags"].append(tag);
            }
        }

        std::string imagesStr = row["images"].as<std::string>();
        if (!imagesStr.empty()) {
            std::istringstream iss(imagesStr);
            std::string imgPath;
            while (std::getline(iss, imgPath, ',')) {
                std::string base64 = loadImageAsBase64(imgPath);
                if (!base64.empty()) {
                    post["img"].append(base64);
                }
            }
        }

        post["createdAt"] = row["created_at"].as<std::string>();
        post["likesCount"] = row["likesCount"].as<int>();
        post["dislikesCount"] = row["dislikesCount"].as<int>();
        posts.append(post);
    }
    return posts;
}

static auto sendPostsResponse(Callback callback) {
    return [callback](const drogon::orm::Result &r) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(buildPostsJson(r));
        resp->setStatusCode(k200OK);
        callback(resp);
    };
}

static void saveImages(int postId, const Json::Value &imgArray, const Json::Value &postJson,
                       std::function<void(const drogon::HttpResponsePtr &)> callback) {
    LOG_INFO << "saveImages called, postId=" << postId << ", imgArray size=" << imgArray.size();
    try {
        if (imgArray.empty()) {
            LOG_INFO << "No images, sending response";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(postJson);
            resp->setStatusCode(k200OK);
            callback(resp);
            return;
        }

        const std::string mediaDir = "../media/";
        if (!std::filesystem::exists(mediaDir)) {
            std::filesystem::create_directory(mediaDir);
        }

        auto db = getDbClient();
        for (const auto &img : imgArray) {
            if (!img.isString()) {
                LOG_ERROR << "Invalid image entry (not a string)";
                sendBadRequest("Invalid image entry", callback);
                return;
            }
            std::string base64 = img.asString();
            std::string filename = generateFilename(".jpg");
            std::string filePath = mediaDir + filename;
            LOG_INFO << "Saving image to " << filePath;
            if (!saveBase64(base64, filePath)) {
                LOG_ERROR << "Failed to save base64 image to " << filePath;
                sendInternalError(callback);
                return;
            }
            db->execSqlAsync(
                "INSERT INTO media (id_post, img) VALUES ($1, $2)",
                [callback, postJson](const drogon::orm::Result &) {
                    LOG_INFO << "Insert successful, sending response";
                },
                [callback](const drogon::orm::DrogonDbException &e) {
                    LOG_ERROR << "Insert error: " << e.base().what();
                    sendInternalError(callback);
                    return;
                },
                postId, filePath);
        }
        LOG_INFO << "All images saved, sending response";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(postJson);
        resp->setStatusCode(k200OK);
        callback(resp);
    } catch (const std::exception &e) {
        LOG_ERROR << "Exception in saveImages: " << e.what();
        sendInternalError(callback);
    }
}

static void setReaction(const HttpRequestPtr &req,
                        std::function<void(const HttpResponsePtr &)> &&callback, std::string postId,
                        bool isLike) {
    verifyToken(req, [callback, req, postId, isLike](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }
        std::string currentLogin = *loginOpt;

        auto db = getDbClient();
        db->execSqlAsync(
            R"sql(SELECT p.id FROM posts p JOIN users u ON u.id = p.author_id WHERE p.id_uuid = $1 
            AND (u.is_public = true OR u.login = $2 OR EXISTS (SELECT 1 FROM friends 
            WHERE id_friend = u.id AND id_user = (SELECT id FROM users WHERE login = $2))))sql",
            [callback, postId, currentLogin, isLike, db](const drogon::orm::Result &r) {
                if (r.empty()) {
                    sendNotFound("Post not found or access denied", callback);
                    return;
                }
                db->execSqlAsync(
                    R"sql(INSERT INTO likes (id_post, author_id, is_like) 
                    VALUES ((SELECT id FROM posts WHERE id_uuid = $1), (SELECT id FROM users WHERE login = $2), $3) 
                    ON CONFLICT (id_post, author_id) DO UPDATE SET is_like = $3)sql",
                    [callback, postId, isLike](const drogon::orm::Result &) {
                        auto db2 = getDbClient();
                        db2->execSqlAsync(
                            R"sql(
                                SELECT p.id_uuid, p.content, (SELECT login FROM users u WHERE u.id = p.author_id) as author, 
                                       (SELECT id_uuid FROM users u WHERE u.id = p.author_id) as author_id, p.created_at,
                                       (SELECT string_agg(tag, ',') FROM tags WHERE id_post = p.id) as tags1,
                                       (SELECT string_agg(img, ',') FROM media WHERE id_post = p.id) as images,
                                       (SELECT COUNT(*) FROM likes WHERE id_post = p.id AND is_like = true) as likesCount,
                                       (SELECT COUNT(*) FROM likes WHERE id_post = p.id AND is_like = false) as dislikesCount
                                FROM posts p WHERE p.id_uuid = $1
                            )sql",
                            [callback](const drogon::orm::Result &r) {
                                if (r.empty()) {
                                    sendNotFound("Post not found", callback);
                                    return;
                                }
                                Json::Value result;
                                auto row = r[0];
                                result["id"] = row["id_uuid"].as<std::string>();
                                result["content"] = row["content"].as<std::string>();
                                result["author"] = row["author"].as<std::string>();
                                result["author_id"] = row["author_id"].as<std::string>();

                                std::string tagsStr = row["tags1"].as<std::string>();
                                if (!tagsStr.empty()) {
                                    std::istringstream iss(tagsStr);
                                    std::string tag;
                                    while (std::getline(iss, tag, ',')) {
                                        result["tags"].append(tag);
                                    }
                                }

                                std::string imagesStr = row["images"].as<std::string>();
                                if (!imagesStr.empty()) {
                                    std::istringstream iss(imagesStr);
                                    std::string imgPath;
                                    while (std::getline(iss, imgPath, ',')) {
                                        std::string base64 = loadImageAsBase64(imgPath);
                                        if (!base64.empty()) {
                                            result["img"].append(base64);
                                        }
                                    }
                                }
                                result["createdAt"] = row["created_at"].as<std::string>();
                                result["likesCount"] = (int)row["likesCount"].as<int64_t>();
                                result["dislikesCount"] = (int)row["dislikesCount"].as<int64_t>();
                                auto resp = HttpResponse::newHttpJsonResponse(result);
                                resp->setStatusCode(k200OK);
                                callback(resp);
                            },
                            sendDbErrorResponse(callback), postId);
                    },
                    sendDbErrorResponse(callback), postId, currentLogin, isLike);
            },
            sendDbErrorResponse(callback), postId, currentLogin);
    });
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
        if (!tags.isArray() || content.empty() || content.size() > 2000) {
            LOG_INFO << content.size();
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

        auto imgArray = json->get("img", Json::arrayValue);
        if (!imgArray.isArray()) {
            sendBadRequest("Invalid img field", callback);
            return;
        }

        auto db = getDbClient();

        db->execSqlAsync(
            R"sql(INSERT INTO posts (content, author_id, created_at) VALUES ($1, (SELECT id FROM users WHERE login = $2), 
            CURRENT_TIMESTAMP) RETURNING id, id_uuid, (SELECT id_uuid FROM users WHERE login = $2) as author_id, created_at)sql",
            [callback, tags, login, content, imgArray](const drogon::orm::Result &r) {
                if (r.empty()) {
                    Json::Value ret;
                    ret["reason"] = "Post creation failed";
                    auto resp = HttpResponse::newHttpJsonResponse(ret);
                    resp->setStatusCode(k500InternalServerError);
                    callback(resp);
                    return;
                }
                int postId = r[0]["id"].as<int>();
                std::string author_id = r[0]["author_id"].as<std::string>();
                std::string uuid = r[0]["id_uuid"].as<std::string>();
                std::string createdAt = r[0]["created_at"].as<std::string>();

                auto db2 = getDbClient();
                for (const auto &tag : tags) {
                    db2->execSqlAsync(R"sql(INSERT INTO tags (id_post, tag) VALUES ($1, $2))sql",
                                      [](const drogon::orm::Result &) {},
                                      [](const drogon::orm::DrogonDbException &e) {
                                          LOG_ERROR << e.base().what();
                                      },
                                      postId, tag.asString());
                }

                Json::Value post;
                post["id"] = uuid;
                post["content"] = content;
                post["author"] = login;
                post["author_id"] = author_id;
                for (const auto &tag : tags) {
                    post["tags"].append(tag.asString());
                }
                post["createdAt"] = createdAt;
                post["likesCount"] = 0;
                post["dislikesCount"] = 0;

                saveImages(postId, imgArray, post, callback);
            },
            [callback](const drogon::orm::DrogonDbException &e) {
                LOG_ERROR << e.base().what();
                Json::Value ret;
                ret["reason"] = "Post creation failed";

                auto resp = HttpResponse::newHttpJsonResponse(ret);
                resp->setStatusCode(k500InternalServerError);
                callback(resp);
            },
            content, login);
    });
}

void PostsController::getPost(const HttpRequestPtr &req, Callback &&callback, std::string postId) {
    verifyToken(req, [callback, req, postId](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }
        std::string currentLogin = *loginOpt;

        fetchPost(postId, currentLogin, [callback, postId](const Json::Value &post, int status) {
            if (status == 404) {
                sendNotFound("The post is not found", callback);
                return;
            }
            if (status != 200) {
                sendInternalError(callback);
                return;
            }

            auto resp = HttpResponse::newHttpJsonResponse(post);
            resp->setStatusCode(k200OK);
            callback(resp);
        });
    });
}

void PostsController::myFeed(const HttpRequestPtr &req, Callback &&callback) {
    verifyToken(req, [callback, req](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }
        std::string currentLogin = *loginOpt;
        int limit = parseLimit(req);

        if (limit < 0) {
            sendBadRequest("limit is incorrect", callback);
            return;
        }

        std::string cursor = req->getParameter("cursor").empty() ? "" : req->getParameter("cursor");
        std::string lastTimestamp;
        std::string lastId;
        if (!cursor.empty()) {
            size_t colonPos = cursor.rfind(':');
            if (colonPos != std::string::npos) {
                lastTimestamp = cursor.substr(0, colonPos);
                try {
                    lastId = cursor.substr(colonPos + 1);
                } catch (...) {
                    sendBadRequest("Invalid cursor format", callback);
                    return;
                }
            } else {
                sendBadRequest("Invalid cursor format", callback);
                return;
            }
        }

        auto db = getDbClient();

        std::string baseSql = R"sql(
            SELECT p.id_uuid, p.content, p.created_at, u.login as author, u.id_uuid as author_id, u.image as author_avatar,
                    (SELECT string_agg(tag, ',') FROM tags WHERE id_post = p.id) as tags1,
                    (SELECT string_agg(img, ',') FROM media WHERE id_post = p.id) as images,
                    (SELECT COUNT(*) FROM likes WHERE id_post = p.id AND is_like = true) as likesCount,
                    (SELECT COUNT(*) FROM likes WHERE id_post = p.id AND is_like = false) as dislikesCount
            FROM posts p
            JOIN users u ON u.id = p.author_id
            WHERE p.author_id = (SELECT id FROM users WHERE login = $1)
        )sql";

        std::string orderLimit = R"sql( ORDER BY p.created_at DESC, p.id DESC LIMIT $)sql" +
                                 std::to_string(cursor.empty() ? 2 : 4);
        ;

        std::string sql;
        if (cursor.empty()) {
            sql = baseSql + orderLimit;
            db->execSqlAsync(sql, sendPostsResponse(callback), sendDbErrorResponse(callback),
                             currentLogin, std::to_string(limit));
        } else {
            sql =
                baseSql + " AND (p.created_at, p.id_uuid) < ($2::timestamp, $3::uuid)" + orderLimit;
            db->execSqlAsync(sql, sendPostsResponse(callback), sendDbErrorResponse(callback),
                             currentLogin, lastTimestamp, lastId, std::to_string(limit));
        }
    });
}

void PostsController::userFeed(const HttpRequestPtr &req, Callback &&callback,
                               std::string targetUserId) {
    verifyToken(req, [callback, req, targetUserId](std::optional<std::string> currentLoginOpt) {
        if (!currentLoginOpt) {
            sendUnauthorized(callback);
            return;
        }
        std::string currentLogin = *currentLoginOpt;

        int limit = parseLimit(req);
        if (limit < 0) {
            sendBadRequest("limit is incorrect", callback);
            return;
        }

        std::string cursor = req->getParameter("cursor").empty() ? "" : req->getParameter("cursor");
        std::string lastTimestamp;
        std::string lastId;
        if (!cursor.empty()) {
            size_t colonPos = cursor.rfind(':');
            if (colonPos != std::string::npos) {
                lastTimestamp = cursor.substr(0, colonPos);
                lastId = cursor.substr(colonPos + 1);
            } else {
                sendBadRequest("Invalid cursor format", callback);
                return;
            }
        }

        auto db = getDbClient();
        db->execSqlAsync(
            R"sql(SELECT id, is_public, login FROM users WHERE id_uuid = $1::uuid)sql",
            [callback, db, currentLogin, targetUserId, limit, cursor, lastTimestamp,
             lastId](const drogon::orm::Result &r) {
                if (r.empty()) {
                    sendNotFound("User not found", callback);
                    return;
                }
                int targetIntId = r[0]["id"].as<int>();
                bool isPublic = r[0]["is_public"].as<bool>();
                std::string targetLogin = r[0]["login"].as<std::string>();

                auto fetchPosts = [callback, db, targetIntId, limit, cursor, lastTimestamp,
                                   lastId]() {
                    std::string baseSql = R"sql(
                            SELECT p.id_uuid, p.content, p.created_at, u.login as author, u.id_uuid as author_id, u.image as author_avatar,
                                   (SELECT string_agg(tag, ',') FROM tags WHERE id_post = p.id) as tags1, 
                                   (SELECT string_agg(img, ',') FROM media WHERE id_post = p.id) as images,
                                   (SELECT COUNT(*) FROM likes WHERE id_post = p.id AND is_like = true) as likesCount,
                                   (SELECT COUNT(*) FROM likes WHERE id_post = p.id AND is_like = false) as dislikesCount
                            FROM posts p
                            JOIN users u ON u.id = p.author_id
                            WHERE p.author_id = $1
                        )sql";

                    std::string orderLimit = " ORDER BY p.created_at DESC, p.id DESC LIMIT $" +
                                             std::to_string(cursor.empty() ? 2 : 4);

                    if (cursor.empty()) {
                        std::string sql = baseSql + orderLimit;
                        db->execSqlAsync(sql, sendPostsResponse(callback),
                                         sendDbErrorResponse(callback), targetIntId,
                                         std::to_string(limit));
                    } else {
                        std::string sql =
                            baseSql + " AND (p.created_at, p.id_uuid) < ($2::timestamp, $3::uuid)" +
                            orderLimit;
                        db->execSqlAsync(sql, sendPostsResponse(callback),
                                         sendDbErrorResponse(callback), targetIntId, lastTimestamp,
                                         lastId, std::to_string(limit));
                    }
                };

                if (currentLogin != targetLogin && !isPublic) {
                    db->execSqlAsync(
                        R"sql(SELECT id FROM friends WHERE id_user = $1 
                                    AND id_friend = (SELECT id FROM users WHERE login = $2))sql",
                        [callback, fetchPosts](const drogon::orm::Result &r) {
                            if (r.empty()) {
                                sendForbidden("You are not allowed to see this profile", callback);
                                return;
                            }
                            fetchPosts();
                        },
                        sendDbErrorResponse(callback), targetIntId, currentLogin);
                } else {
                    fetchPosts();
                }
            },
            sendDbErrorResponse(callback), targetUserId);
    });
}

void PostsController::newsFriendsFeed(const HttpRequestPtr &req, Callback &&callback) {
    verifyToken(req, [callback, req](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }

        int limit = parseLimit(req);
        if (limit < 0) {
            sendBadRequest("limit is incorrect", callback);
            return;
        }

        std::string cursor = req->getParameter("cursor").empty() ? "" : req->getParameter("cursor");
        std::string lastTimestamp;
        std::string lastId;
        if (!cursor.empty()) {
            size_t colonPos = cursor.rfind(':');
            if (colonPos != std::string::npos) {
                lastTimestamp = cursor.substr(0, colonPos);
                try {
                    lastId = cursor.substr(colonPos + 1);
                } catch (...) {
                    sendBadRequest("Invalid cursor format", callback);
                    return;
                }
            } else {
                sendBadRequest("Invalid cursor format", callback);
                return;
            }
        }

        std::string currentLogin = *loginOpt;
        auto db = getDbClient();

        std::string baseSql = R"sql(
            WITH curr AS (
                SELECT id FROM users WHERE login = $1
            )
            SELECT p.id_uuid, p.content, p.created_at, u.login as author, u.id_uuid as author_id, u.image as author_avatar,
                   (SELECT string_agg(tag, ',') FROM tags WHERE id_post = p.id) as tags1,
                   (SELECT string_agg(img, ',') FROM media WHERE id_post = p.id) as images,
                   (SELECT COUNT(*) FROM likes WHERE id_post = p.id AND is_like = true) as likesCount,
                   (SELECT COUNT(*) FROM likes WHERE id_post = p.id AND is_like = false) as dislikesCount
            FROM posts p
            JOIN users u ON u.id = p.author_id
            WHERE (
                u.login = $1
                OR (EXISTS (
                        SELECT 1 FROM friends f
                        WHERE f.id_friend = u.id
                          AND f.id_user = (SELECT id FROM curr)
                    )
                    AND (u.is_public = true
                         OR EXISTS (
                             SELECT 1 FROM friends f
                             WHERE f.id_friend = (SELECT id FROM curr)
                               AND f.id_user = u.id
                         )
                    )
                )
            )
        )sql";

        std::string orderLimit = " ORDER BY p.created_at DESC, p.id DESC LIMIT $" +
                                 std::to_string(cursor.empty() ? 2 : 4);

        if (cursor.empty()) {
            std::string sql = baseSql + orderLimit;
            db->execSqlAsync(sql, sendPostsResponse(callback), sendDbErrorResponse(callback),
                             currentLogin, std::to_string(limit));
        } else {
            std::string sql =
                baseSql + " AND (p.created_at, p.id_uuid) < ($2::timestamp, $3::uuid)" + orderLimit;
            db->execSqlAsync(sql, sendPostsResponse(callback), sendDbErrorResponse(callback),
                             currentLogin, lastTimestamp, lastId, std::to_string(limit));
        }
    });
}

void PostsController::newsFeed(const HttpRequestPtr &req, Callback &&callback) {
    verifyToken(req, [callback, req](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }

        int limit = parseLimit(req);
        if (limit < 0) {
            sendBadRequest("limit is incorrect", callback);
            return;
        }

        LOG_INFO << "got limit" << limit;

        std::string cursor = req->getParameter("cursor").empty() ? "" : req->getParameter("cursor");
        LOG_INFO << cursor;
        std::string lastTimestamp;
        std::string lastId;
        if (!cursor.empty()) {
            size_t colonPos = cursor.rfind(':');
            if (colonPos != std::string::npos) {
                lastTimestamp = cursor.substr(0, colonPos);
                try {
                    lastId = cursor.substr(colonPos + 1);
                } catch (...) {
                    sendBadRequest("Invalid cursor format", callback);
                    return;
                }
            } else {
                sendBadRequest("Invalid cursor format", callback);
                return;
            }
        }
        LOG_INFO << "got cursor";

        std::string currentLogin = *loginOpt;
        auto db = getDbClient();

        std::string baseSql = R"sql(
            SELECT p.id_uuid, p.content, p.created_at, u.login as author, u.id_uuid as author_id, u.image as author_avatar,
                   (SELECT string_agg(tag, ',') FROM tags WHERE id_post = p.id) as tags1,
                   (SELECT string_agg(img, ',') FROM media WHERE id_post = p.id) as images,
                   (SELECT COUNT(*) FROM likes WHERE id_post = p.id AND is_like = true) as likesCount,
                   (SELECT COUNT(*) FROM likes WHERE id_post = p.id AND is_like = false) as dislikesCount
            FROM posts p
            JOIN users u ON u.id = p.author_id
            WHERE (
                u.is_public = true
                OR u.login = $1
                OR EXISTS (
                    SELECT 1 FROM friends f
                    WHERE f.id_user = u.id
                      AND f.id_friend = (SELECT id FROM users WHERE login = $1)
                )
            )
        )sql";

        std::string orderLimit = " ORDER BY p.created_at DESC, p.id DESC LIMIT $" +
                                 std::to_string(cursor.empty() ? 2 : 4);

        if (cursor.empty()) {
            LOG_INFO << "cursor is empty";
            std::string sql = baseSql + orderLimit;
            db->execSqlAsync(sql, sendPostsResponse(callback), sendDbErrorResponse(callback),
                             currentLogin, std::to_string(limit));
        } else {
            LOG_INFO << "cursor is not empty";
            std::string sql =
                baseSql + " AND (p.created_at, p.id_uuid) < ($2::timestamp, $3::uuid)" + orderLimit;
            db->execSqlAsync(sql, sendPostsResponse(callback), sendDbErrorResponse(callback),
                             currentLogin, lastTimestamp, lastId, std::to_string(limit));
        }
    });
}

void PostsController::likePost(const HttpRequestPtr &req, Callback &&callback, std::string postId) {
    setReaction(req, std::move(callback), postId, true);
}

void PostsController::dislikePost(const HttpRequestPtr &req, Callback &&callback,
                                  std::string postId) {
    setReaction(req, std::move(callback), postId, false);
}