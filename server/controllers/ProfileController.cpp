#include "ProfileController.hpp"
#include "helpers.hpp"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>

using namespace drogon;

void ProfileController::getProfile(const HttpRequestPtr &req, Callback &&callback,
                                   std::string login) {
    verifyToken(req, [callback, login](std::optional<std::string> loginOpt) mutable {
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }
        std::string currentLogin = *loginOpt;
        if (login == "me")
            login = currentLogin;
        bool is_me = (login == currentLogin ? true : false);

        auto db = getDbClient();
        db->execSqlAsync(
            R"sql(
                WITH target_user AS (SELECT id, login, image, is_public FROM users WHERE login = $1),
                     counts AS (
                         SELECT
                             (SELECT COUNT(*) FROM friends WHERE id_friend = (SELECT id FROM target_user)) AS followers_count,
                             (SELECT COUNT(*) FROM friends WHERE id_user = (SELECT id FROM target_user)) AS following_count,
                             (SELECT COUNT(*) FROM posts WHERE author_id = (SELECT id FROM users WHERE login = $1)) AS posts_count
                     ),
                     relations AS (
                         SELECT
                             EXISTS (SELECT 1 FROM friends WHERE id_user = (SELECT id FROM users WHERE login = $2) AND id_friend = (SELECT id FROM target_user)) AS is_following,
                             EXISTS (SELECT 1 FROM friends WHERE id_user = (SELECT id FROM target_user) AND id_friend = (SELECT id FROM users WHERE login = $2)) AS is_followed_by
                     )
                SELECT
                    (SELECT login FROM target_user) AS login,
                    (SELECT image FROM target_user) AS image,
                    (SELECT is_public FROM target_user) AS is_public,
                    (SELECT followers_count FROM counts) AS followers_count,
                    (SELECT following_count FROM counts) AS following_count,
                    (SELECT posts_count FROM counts) AS posts_count,
                    (SELECT is_following FROM relations) AS is_following,
                    (SELECT is_followed_by FROM relations) AS is_followed_by
            )sql",
            [callback, currentLogin, login, is_me](const drogon::orm::Result &r) {
                if (r.empty()) {
                    sendNotFound("User not found", callback);
                    return;
                }
                auto row = r[0];
                bool isPublic = row["is_public"].as<bool>();

                std::string imagePath = row["image"].as<std::string>();
                std::string imageBase64;
                if (!imagePath.empty()) {
                    imageBase64 = loadImageAsBase64(imagePath);
                }

                Json::Value profile;
                profile["login"] = row["login"].as<std::string>();
                if (!imageBase64.empty()) {
                    profile["image"] = imageBase64;
                } else {
                    profile["image"] = Json::nullValue;
                }
                profile["followersCount"] = (int)row["followers_count"].as<int64_t>();
                profile["followingCount"] = (int)row["following_count"].as<int64_t>();
                profile["postsCount"] = (int)row["posts_count"].as<int64_t>();
                profile["isFollowing"] =
                    row["is_following"]
                        .as<bool>(); // подписан ли текущий пользователь на пользователя с login
                profile["isFollowedBy"] =
                    row["is_followed_by"]
                        .as<bool>(); // подписан ли пользователь c login на текущего пользователя
                profile["isPublic"] = isPublic;
                profile["isMe"] = is_me;
                profile["allowedToSee"] = true;
                if (currentLogin != login && !isPublic && !row["is_followed_by"].as<bool>()) {
                    profile["allowedToSee"] = false;
                }
                LOG_INFO << login << " " << currentLogin;
                LOG_INFO << profile.toStyledString();
                auto resp = HttpResponse::newHttpJsonResponse(profile);
                resp->setStatusCode(k200OK);
                callback(resp);
            },
            sendDbErrorResponse(callback), login, currentLogin);
    });
}

void ProfileController::updatePassword(const HttpRequestPtr &req, Callback &&callback) {
    verifyToken(req, [callback, req](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }
        std::string login = *loginOpt;

        auto json = req->getJsonObject();
        if (!json || !(*json).isMember("oldPassword") || !(*json).isMember("newPassword")) {
            sendBadRequest("Missing oldPassword or newPassword", callback);
            return;
        }
        std::string oldPassword = (*json)["oldPassword"].asString();
        std::string newPassword = (*json)["newPassword"].asString();

        if (!validatePasswordStrength(newPassword)) {
            sendBadRequest("Weak password", callback);
            return;
        }

        auto db = getDbClient();
        db->execSqlAsync(
            "SELECT password, token_number FROM users WHERE login = $1",
            [callback, login, oldPassword, newPassword, db](const drogon::orm::Result &r) {
                if (r.empty()) {
                    sendNotFound("User not found", callback);
                    return;
                }
                auto row = r[0];
                std::string hashed = row["password"].as<std::string>();
                if (!checkPassword(oldPassword, hashed)) {
                    sendBadRequest("The old password is not correct", callback);
                    return;
                }
                std::string newHashed = hashPassword(newPassword);
                int currentTokenNumber = row["token_number"].as<int>();
                int newTokenNumber = currentTokenNumber + 1;

                db->execSqlAsync(
                    "UPDATE users SET password = $1, token_number = $2 WHERE login = $3",
                    [callback](const drogon::orm::Result &) {
                        Json::Value ret;
                        ret["status"] = "ok";
                        auto resp = HttpResponse::newHttpJsonResponse(ret);
                        resp->setStatusCode(k200OK);
                        callback(resp);
                    },
                    sendDbErrorResponse(callback), newHashed, newTokenNumber, login);
            },
            sendDbErrorResponse(callback), login);
    });
}

void ProfileController::updateMyProfile(const HttpRequestPtr &req, Callback &&callback) {
    verifyToken(req, [callback, req](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }
        std::string currentLogin = *loginOpt;

        auto json = req->getJsonObject();
        if (!json) {
            sendBadRequest("Invalid JSON", callback);
            return;
        }

        std::string newLogin, newEmail, newPhone, newImage;
        bool newIsPublic = false;
        bool hasIsPublic = false;

        if (json->isMember("login")) {
            newLogin = (*json)["login"].asString();
            if (!validateLogin(newLogin)) {
                sendBadRequest("Incorrect login format", callback);
                return;
            }
        }
        if (json->isMember("email")) {
            newEmail = (*json)["email"].asString();
            if (!validateEmail(newEmail)) {
                sendBadRequest("Incorrect email format", callback);
                return;
            }
        }
        if (json->isMember("phone")) {
            newPhone = (*json)["phone"].asString();
            if (!validatePhone(newPhone)) {
                sendBadRequest("Incorrect phone format", callback);
                return;
            }
        }
        if (json->isMember("image")) {
            newImage = (*json)["image"].asString();
            if (!validateImage(newImage) && !newImage.empty()) {
                sendBadRequest("Incorrect image format", callback);
                return;
            }
        }
        if (json->isMember("isPublic")) {
            newIsPublic = (*json)["isPublic"].asBool();
            hasIsPublic = true;
        }

        if (newLogin.empty() && newEmail.empty() && newPhone.empty() && newImage.empty() &&
            !hasIsPublic) {
            sendBadRequest("No fields to update", callback);
            return;
        }

        auto db = getDbClient();

        std::string oldImagePath;
        std::string newImagePath;

        auto getOldImage = [&](std::function<void()> next) {
            if (newImage.empty()) {
                next();
                return;
            }
            db->execSqlAsync(
                "SELECT image FROM users WHERE login = $1",
                [&oldImagePath, callback, next](const drogon::orm::Result &r) {
                    if (!r.empty()) {
                        oldImagePath = r[0]["image"].as<std::string>();
                    }
                    next();
                },
                sendDbErrorResponse(callback), currentLogin);
        };

        auto saveImage = [&](std::function<void()> next) {
            if (newImage.empty()) {
                next();
                return;
            }
            const std::string mediaDir = "../media/";
            if (!std::filesystem::exists(mediaDir))
                std::filesystem::create_directory(mediaDir);
            std::string filename = generateFilename(".jpg");
            std::string filePath = mediaDir + filename;
            if (!saveBase64(newImage, filePath)) {
                sendInternalError(callback);
                return;
            }
            if (!oldImagePath.empty() && std::filesystem::exists(oldImagePath)) {
                std::filesystem::remove(oldImagePath);
            }
            newImagePath = filePath;
            next();
        };

        auto checkLogin = [=](std::function<void()> next) {
            if (newLogin.empty()) {
                next();
                return;
            }
            db->execSqlAsync(
                "SELECT login FROM users WHERE login = $1 AND login != $2",
                [callback, next](const drogon::orm::Result &r) {
                    if (!r.empty()) {
                        sendBadRequest("Login is already taken", callback);
                        return;
                    }
                    next();
                },
                sendDbErrorResponse(callback), newLogin, currentLogin);
        };

        auto checkEmail = [=](std::function<void()> next) {
            if (newEmail.empty()) {
                next();
                return;
            }
            db->execSqlAsync(
                "SELECT email FROM users WHERE email = $1 AND login != $2",
                [callback, next](const drogon::orm::Result &r) {
                    if (!r.empty()) {
                        sendBadRequest("Email is already taken", callback);
                        return;
                    }
                    next();
                },
                sendDbErrorResponse(callback), newEmail, currentLogin);
        };

        auto checkPhone = [=](std::function<void()> next) {
            if (newPhone.empty()) {
                next();
                return;
            }
            db->execSqlAsync(
                "SELECT phone FROM users WHERE phone = $1 AND login != $2",
                [callback, next](const drogon::orm::Result &r) {
                    if (!r.empty()) {
                        sendBadRequest("Phone is already taken", callback);
                        return;
                    }
                    next();
                },
                sendDbErrorResponse(callback), newPhone, currentLogin);
        };

        auto doUpdate = [=]() {
            std::vector<std::string> updates;
            std::vector<std::string> values;

            if (!newLogin.empty()) {
                updates.push_back("login = $" + std::to_string(updates.size() + 1));
                values.push_back(newLogin);
            }
            if (!newEmail.empty()) {
                updates.push_back("email = $" + std::to_string(updates.size() + 1));
                values.push_back(newEmail);
            }
            if (!newPhone.empty()) {
                updates.push_back("phone = $" + std::to_string(updates.size() + 1));
                values.push_back(newPhone);
            }
            if (!newImagePath.empty()) {
                updates.push_back("image = $" + std::to_string(updates.size() + 1));
                values.push_back(newImagePath);
            }
            if (hasIsPublic) {
                updates.push_back("is_public = $" + std::to_string(updates.size() + 1));
                values.push_back(newIsPublic ? "true" : "false");
            }

            if (updates.empty()) {
                sendBadRequest("No fields to update", callback);
                return;
            }

            std::string sql = "UPDATE users SET " + updates[0];
            for (size_t i = 1; i < updates.size(); ++i) {
                sql += ", " + updates[i];
            }
            sql += " WHERE login = $" + std::to_string(updates.size() + 1) +
                   " RETURNING login, email, phone, image, is_public";
            values.push_back(currentLogin);
            size_t paramCount = values.size();

            auto onSuccess = [callback, newImage](const drogon::orm::Result &r) {
                if (r.empty()) {
                    sendNotFound("User not found", callback);
                    return;
                }
                auto row = r[0];
                Json::Value profile;
                profile["login"] = row["login"].as<std::string>();
                profile["email"] = row["email"].as<std::string>();
                profile["phone"] = row["phone"].as<std::string>();
                if (!newImage.empty()) {
                    profile["image"] = newImage;
                } else {
                    std::string imagePath = row["image"].as<std::string>();
                    std::string imageBase64;
                    if (!imagePath.empty())
                        imageBase64 = loadImageAsBase64(imagePath);
                    if (!imageBase64.empty())
                        profile["image"] = imageBase64;
                    else
                        profile["image"] = Json::nullValue;
                }
                profile["isPublic"] = row["is_public"].as<bool>();
                auto resp = HttpResponse::newHttpJsonResponse(profile);
                resp->setStatusCode(k200OK);
                callback(resp);
            };
            auto onError = [callback](const drogon::orm::DrogonDbException &e) {
                LOG_ERROR << e.base().what();
                sendInternalError(callback);
            };

            switch (paramCount) {
                case 2:
                    db->execSqlAsync(sql, onSuccess, onError, values[0], values[1]);
                    break;
                case 3:
                    db->execSqlAsync(sql, onSuccess, onError, values[0], values[1], values[2]);
                    break;
                case 4:
                    db->execSqlAsync(sql, onSuccess, onError, values[0], values[1], values[2],
                                     values[3]);
                    break;
                case 5:
                    db->execSqlAsync(sql, onSuccess, onError, values[0], values[1], values[2],
                                     values[3], values[4]);
                    break;
                case 6:
                    db->execSqlAsync(sql, onSuccess, onError, values[0], values[1], values[2],
                                     values[3], values[4], values[5]);
                    break;
                default:
                    sendInternalError(callback);
                    return;
            }
        };

        auto afterSaveImage = [=]() { saveImage(doUpdate); };
        auto afterGetOldImage = [=]() { getOldImage(afterSaveImage); };
        auto afterPhone = [=]() { checkPhone(doUpdate); };
        auto afterEmail = [=]() { checkEmail(afterPhone); };
        auto afterLogin = [=]() { checkLogin(afterEmail); };

        afterLogin();
    });
}