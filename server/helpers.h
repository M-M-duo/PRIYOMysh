#pragma once

#include <drogon/drogon.h>
#include <crypt.h>
#include <string>

using namespace drogon;

using Callback = std::function<void(const HttpResponsePtr &)>;
const std::string DB_CLIENT = "postgres";

inline auto getDbClient() {
    return app().getDbClient(DB_CLIENT);
}

inline std::string hashPassword(const std::string& plain) {
    char salt[32];
    strcpy(salt, "$2b$10$");
    struct crypt_data data;
    data.initialized = 0;
    char* hash = crypt_r(plain.c_str(), salt, &data);
    return std::string(hash);
}