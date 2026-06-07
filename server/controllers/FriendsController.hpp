#pragma once

#include <drogon/HttpController.h>

class FriendsController : public drogon::HttpController<FriendsController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(FriendsController::addFriend, "/api/friends/add", drogon::Post);
        ADD_METHOD_TO(FriendsController::removeFriend, "/api/friends/remove", drogon::Post);
        ADD_METHOD_TO(FriendsController::getFriendsList, "/api/friends", drogon::Get);
    METHOD_LIST_END

    void addFriend(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void removeFriend(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getFriendsList(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};