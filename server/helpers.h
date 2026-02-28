#pragma once

#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>
#include <crypt.h>
#include <string>

using namespace drogon;

using Callback = std::function<void(const HttpResponsePtr &)>;

inline drogon::orm::DbClientPtr getDbClient() {
    static drogon::orm::DbClientPtr client = drogon::orm::DbClient::newPgClient(
    "host=" + std::string(std::getenv("POSTGRES_HOST")) +
    " port=" + std::string(std::getenv("POSTGRES_PORT")) +
    " dbname=" + std::string(std::getenv("POSTGRES_DATABASE")) +
    " user=" + std::string(std::getenv("POSTGRES_USERNAME")) +
    " password=" + std::string(std::getenv("POSTGRES_PASSWORD")),
    1);
    return client;
}

inline std::string hashPassword(const std::string& plain) {
    char salt[128];
    char hash[128];
    struct crypt_data data;
    data.initialized = 0;

    if (!crypt_gensalt_r("$2b$", 10, nullptr, 0, salt, sizeof(salt))) {
        LOG_ERROR << "crypt_gensalt_r failed";
        return "";
    }

    char* result = crypt_r(plain.c_str(), salt, &data);
    if (!result) {
        LOG_ERROR << "crypt_r returned NULL";
        return "";
    }
    return std::string(result);
}

inline bool validateLogin(const std::string& login) {
    return !login.empty() && login.length() <= 30 &&
           std::regex_match(login, std::regex("^[a-zA-Z0-9-]+$"));
}

inline bool validateEmail(const std::string& email) {
    if (email.length() > 50) return false;
    const std::regex pattern(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    return std::regex_match(email, pattern);
}

inline bool validatePasswordStrength(const std::string& pw) {
    if (pw.length() < 6 || pw.length() > 100) return false;
    bool upper = false, lower = false, digit = false;
    for (char c : pw) {
        if (isupper(c)) upper = true;
        else if (islower(c)) lower = true;
        else if (isdigit(c)) digit = true;
    }
    return upper && lower && digit;
}

inline bool validatePhone(const std::string& phone) {
    return phone.length() <= 20 && std::regex_match(phone, std::regex("^\\+[\\d]+$"));
}

inline bool validateImage(const std::string& image) {
    return image.length() <= 200;
}