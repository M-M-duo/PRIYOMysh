#pragma once

#include <drogon/HttpController.h>

class ProfileController : public drogon::HttpController<ProfileController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ProfileController::updateMyProfile, "/api/me/profile", drogon::Patch);
    ADD_METHOD_TO(ProfileController::getProfile, "/api/profiles/{idStr}", drogon::Get);
    ADD_METHOD_TO(ProfileController::updatePassword, "/api/me/updatePassword", drogon::Post);
    ADD_METHOD_TO(ProfileController::getProfileToEdit, "/api/me/profile", drogon::Get);
    METHOD_LIST_END

    void updateMyProfile(const drogon::HttpRequestPtr &req,
                         std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void getProfile(const drogon::HttpRequestPtr &req,
                    std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                    std::string idStr);
    void updatePassword(const drogon::HttpRequestPtr &req,
                        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void getProfileToEdit(const drogon::HttpRequestPtr &req,
                          std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};