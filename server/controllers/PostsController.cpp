#include "PostsController.hpp"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <trantor/utils/Date.h>
#include <iomanip>
#include <sstream>
#include "helpers.hpp"

using namespace drogon;

static void fetchPost(
    const std::string &postId,
    const std::string &currentLogin,
    std::function<void(const Json::Value &, int)> callback
) {
    auto db = getDbClient();
    db->execSqlAsync(
        R"sql(SELECT p.*, u.is_public as author_public, 
                     (SELECT string_agg(tag, ',') FROM tags WHERE id_post = p.id) as tags1,
                     (SELECT string_agg(img, ',') FROM media WHERE id_post = p.id) as images
                     FROM posts p JOIN users u ON u.login = p.author WHERE p.id_uuid = $1)sql",
        [callback, currentLogin, db](const drogon::orm::Result &r) {
            if (r.empty()) {
                callback(Json::Value(), 404);
                return;
            }
            auto row = r[0];
            std::string author = row["author"].as<std::string>();
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
                    [callback, row, tags,
                     author](const drogon::orm::Result &r) {
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
                    author, currentLogin
                );
            }

            Json::Value post;
            post["id"] = row["id_uuid"].as<std::string>();
            post["content"] = row["content"].as<std::string>();
            post["author"] = author;
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

static void saveImages(
    int postId,
    const Json::Value &imgArray,
    const Json::Value &postJson,
    std::function<void(const drogon::HttpResponsePtr &)> callback
) {
    LOG_INFO << "saveImages called, postId=" << postId
             << ", imgArray size=" << imgArray.size();
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
                postId, filePath
            );
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

        auto imgArray = json->get("img", Json::arrayValue);
        if (!imgArray.isArray()) {
            sendBadRequest("Invalid img field", callback);
            return;
        }

        auto db = getDbClient();

        db->execSqlAsync(
            R"sql(INSERT INTO posts (content, author, created_at) VALUES ($1, $2, 
            CURRENT_TIMESTAMP) RETURNING id, id_uuid, created_at)sql",
            [callback, tags, login, content,
             imgArray](const drogon::orm::Result &r) {
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
                std::string createdAt = r[0]["created_at"].as<std::string>();

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
            content, login
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
                       (SELECT string_agg(tag, ',') FROM tags WHERE id_post = p.id) as tags1,
                       (SELECT string_agg(img, ',') FROM media WHERE id_post = p.id) as images
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
                        db->execSqlAsync(
                            R"sql(SELECT id FROM friends WHERE id_user = (SELECT id FROM users WHERE login = $1) 
                                    AND id_friend = (SELECT id FROM users WHERE login = $2))sql",
                            [callback, currentLogin,
                             login](const drogon::orm::Result &r) {
                                if (r.empty()) {
                                    sendForbidden(
                                        "You are not allowed to see this "
                                        "profile",
                                        callback
                                    );
                                    return;
                                }
                            },
                            sendDbErrorResponse(callback), login, currentLogin
                        );
                    }

                    db->execSqlAsync(
                        R"sql(
                        SELECT p.id_uuid, p.content, p.author, p.created_at,
                               (SELECT string_agg(tag, ',') FROM tags WHERE id_post = p.id) as tags1, 
                               (SELECT string_agg(img, ',') FROM media WHERE id_post = p.id) as images
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

        std::string currentLogin = *loginOpt;

        auto db = getDbClient();
        db->execSqlAsync(
            R"sql(
                SELECT p.id_uuid, p.content, p.author, p.created_at,
                       (SELECT string_agg(tag, ',') FROM tags WHERE id_post = p.id) as tags1, 
                       (SELECT string_agg(img, ',') FROM media WHERE id_post = p.id) as images
                FROM posts p
                JOIN users u ON u.login = p.author 
                WHERE u.is_public = true OR u.login = $3 OR EXISTS (SELECT 1 FROM friends f WHERE f.id_friend = (SELECT id from users WHERE login = $3) AND f.id_user = u.id)
                ORDER BY p.created_at DESC
                LIMIT $1 OFFSET $2
            )sql",
            sendPostsResponse(callback), sendDbErrorResponse(callback),
            std::to_string(limit), std::to_string(offset), currentLogin
        );
    });
}