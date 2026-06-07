#include "ProfileController.hpp"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include "helpers.hpp"

using namespace drogon;

void ProfileController::getProfile(
    const HttpRequestPtr &req,
    Callback &&callback,
    std::string login
) {
    verifyToken(req, [callback, login](std::optional<std::string> loginOpt) {
        if (!loginOpt) {
            sendUnauthorized(callback);
            return;
        }
        std::string currentLogin = *loginOpt;

        auto db = getDbClient();
        db->execSqlAsync(
            R"sql(
                WITH target_user AS (SELECT id, login, image, is_public FROM users WHERE login = $1),
                     counts AS (
                         SELECT
                             (SELECT COUNT(*) FROM friends WHERE id_friend = (SELECT id FROM target_user)) AS followers_count,
                             (SELECT COUNT(*) FROM friends WHERE id_user = (SELECT id FROM target_user)) AS following_count,
                             (SELECT COUNT(*) FROM posts WHERE author = $1) AS posts_count
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
            [callback, currentLogin, login](const drogon::orm::Result &r) {
                if (r.empty()) {
                    sendNotFound("User not found", callback);
                    return;
                }
                auto row = r[0];
                bool isPublic = row["is_public"].as<bool>();

                Json::Value profile;
                profile["login"] = row["login"].as<std::string>();
                std::string image = row["image"].as<std::string>();
                if (!image.empty()) {
                    profile["image"] = image;
                } else {
                    profile["image"] = Json::nullValue;
                }
                profile["followersCount"] =
                    (int)row["followers_count"].as<int64_t>();
                profile["followingCount"] =
                    (int)row["following_count"].as<int64_t>();
                profile["postsCount"] = (int)row["posts_count"].as<int64_t>();
                profile["isFollowing"] = row["is_following"].as<bool>();
                profile["isFollowedBy"] = row["is_followed_by"].as<bool>();
                profile["isPublic"] = isPublic;
                profile["allowedToSee"] = true;
                if (currentLogin != login && !isPublic &&
                    !row["is_following"].as<bool>()) {
                    profile["allowedToSee"] = false;
                }
                auto resp = HttpResponse::newHttpJsonResponse(profile);
                resp->setStatusCode(k200OK);
                callback(resp);
            },
            sendDbErrorResponse(callback), login, currentLogin
        );
    });
}