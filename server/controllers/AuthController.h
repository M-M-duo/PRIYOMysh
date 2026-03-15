#pragma once

#include <drogon/HttpController.h>

class AuthController : public drogon::HttpController<AuthController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AuthController::ping, "/api/ping", drogon::Get);
        ADD_METHOD_TO(AuthController::registerUser, "/api/auth/register", drogon::Post);
        ADD_METHOD_TO(AuthController::signIn, "/api/auth/sign-in", drogon::Post);
    METHOD_LIST_END

    void ping(const drogon::HttpRequestPtr& req,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void registerUser(const drogon::HttpRequestPtr& req,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void signIn(const drogon::HttpRequestPtr& req,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};