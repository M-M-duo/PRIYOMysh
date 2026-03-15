#pragma once

#include <drogon/HttpController.h>

class PostsController : public drogon::HttpController<PostsController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(PostsController::newPost, "/api/posts/new", drogon::Post);
    METHOD_LIST_END

    void newPost(const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};