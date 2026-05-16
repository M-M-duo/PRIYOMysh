#pragma once

#include <drogon/HttpController.h>

class PostsController : public drogon::HttpController<PostsController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(PostsController::newPost, "/api/posts/new", drogon::Post);
        ADD_METHOD_TO(PostsController::getPost, "/api/posts/{postId}", drogon::Get);
        ADD_METHOD_TO(PostsController::myFeed, "/api/posts/feed/my", drogon::Get);
        ADD_METHOD_TO(PostsController::userFeed, "/api/posts/feed/{login}", drogon::Get);
        ADD_METHOD_TO(PostsController::newsFeed, "/api/posts/feed", drogon::Get);
        ADD_METHOD_TO(PostsController::newsFriendsFeed, "/api/posts/feed/friends", drogon::Get);
    METHOD_LIST_END

    void newPost(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getPost(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                std::string postId);
    void myFeed(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void userFeed(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                std::string login);
    void newsFeed(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void newsFriendsFeed(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};