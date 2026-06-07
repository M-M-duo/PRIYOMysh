#pragma once

#include <drogon/HttpController.h>

class FriendsController : public drogon::HttpController<FriendsController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(FriendsController::addFriend, "/api/friends/add", drogon::Post);
        ADD_METHOD_TO(FriendsController::removeFriend, "/api/friends/remove", drogon::Post);
        ADD_METHOD_TO(FriendsController::getFollowingList, "/api/friends/{login}/following", drogon::Get);
        ADD_METHOD_TO(FriendsController::getFollowersList, "/api/friends/{login}/followers", drogon::Get);
        ADD_METHOD_TO(FriendsController::getUser, "/api/friends/search/{login}", drogon::Get);
    METHOD_LIST_END

    void addFriend(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void removeFriend(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getFollowingList(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                std::string login);
    void getFollowersList(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                std::string login);
    void getUser(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                std::string login);
};