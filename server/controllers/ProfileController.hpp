#pragma once

#include <drogon/HttpController.h>

class ProfileController : public drogon::HttpController<ProfileController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ProfileController::getProfile, "/api/profiles/{login}", drogon::Get);
    METHOD_LIST_END

    void getProfile(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                    std::string login);
};