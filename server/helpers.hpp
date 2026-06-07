#pragma once
#include <crypt.h>
#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>
#include <drogon/utils/Utilities.h>
#include <jwt-cpp/jwt.h>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <optional>
#include <random>
#include <regex>
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
        1
    );
    return client;
}

inline std::string hashPassword(const std::string &plain) {
    char salt[128];
    char hash[128];
    struct crypt_data data;
    data.initialized = 0;

    if (!crypt_gensalt_r("$2b$", 10, nullptr, 0, salt, sizeof(salt))) {
        LOG_ERROR << "crypt_gensalt_r failed";
        return "";
    }

    char *result = crypt_r(plain.c_str(), salt, &data);
    if (!result) {
        LOG_ERROR << "crypt_r returned NULL";
        return "";
    }
    return std::string(result);
}

inline bool checkPassword(const std::string &plain, const std::string &hash) {
    struct crypt_data data;
    data.initialized = 0;
    char *result = crypt_r(plain.c_str(), hash.c_str(), &data);
    return result && hash == result;
}

inline std::string JWT_SECRET = [] {
    auto s = std::getenv("RANDOM_SECRET");
    return s ? s : "default_secret";
}();

inline std::string
createToken(const std::string &login, int token_number, int update_token) {
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::hours(20);
    auto token =
        jwt::create()
            .set_type("JWT")
            .set_payload_claim("login", jwt::claim(login))
            .set_expires_at(exp)
            .set_payload_claim(
                "token_number", jwt::claim(std::to_string(token_number))
            )
            .set_payload_claim(
                "update_token", jwt::claim(std::to_string(update_token))
            )
            .set_issuer("")
            .sign(jwt::algorithm::hs256{JWT_SECRET});
    return token;
}

struct TokenPayload {
    std::string login;
    std::chrono::system_clock::time_point exp;
    int token_number;
    int update_token;
};

inline std::optional<TokenPayload> getTokenContent(const std::string &token) {
    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
                            .allow_algorithm(jwt::algorithm::hs256{JWT_SECRET})
                            .with_issuer("");
        verifier.verify(decoded);

        TokenPayload payload;
        payload.login = decoded.get_payload_claim("login").as_string();
        payload.exp = decoded.get_payload_claim("exp").as_date();
        payload.token_number =
            std::stoi(decoded.get_payload_claim("token_number").as_string());
        payload.update_token =
            std::stoi(decoded.get_payload_claim("update_token").as_string());
        return payload;
    } catch (const std::exception &e) {
        LOG_ERROR << "JWT verification exception: " << e.what();
        return std::nullopt;
    }
}

inline bool validateLogin(const std::string &login) {
    return !login.empty() && login.length() <= 30 &&
           std::regex_match(login, std::regex("^[a-zA-Z0-9-]+$"));
}

inline bool validateEmail(const std::string &email) {
    if (email.length() > 50) {
        return false;
    }
    const std::regex pattern(
        R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)"
    );
    return std::regex_match(email, pattern);
}

inline bool validatePasswordStrength(const std::string &pw) {
    if (pw.length() < 6 || pw.length() > 100) {
        return false;
    }
    bool upper = false, lower = false, digit = false;
    for (char c : pw) {
        if (isupper(c)) {
            upper = true;
        } else if (islower(c)) {
            lower = true;
        } else if (isdigit(c)) {
            digit = true;
        }
    }
    return upper && lower && digit;
}

inline bool validatePhone(const std::string &phone) {
    return phone.length() <= 20 &&
           std::regex_match(phone, std::regex("^\\+[\\d]+$"));
}

inline bool validateImage(const std::string &image) {
    return image.length() <= 200;
}

inline std::string generateFilename(const std::string &extension = ".jpg") {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now.time_since_epoch()
    )
                         .count();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);
    return std::to_string(timestamp) + "_" + std::to_string(dis(gen)) +
           extension;
}

inline bool
saveBase64(const std::string &base64Data, const std::string &filePath) {
    LOG_INFO << "saveBase64 started";
    std::string decoded = drogon::utils::base64Decode(base64Data);
    LOG_INFO << "decoded Base64";
    if (decoded.empty()) {
        LOG_ERROR << "Failed to decode base64 image";
        return false;
    }
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR << "Failed to open file: " << filePath;
        return false;
    }
    file.write(decoded.data(), decoded.size());
    file.close();
    return true;
}

inline std::string loadImageAsBase64(const std::string &filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR << "Failed to open image file: " << filePath;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    return drogon::utils::base64Encode(content);
}

inline void verifyToken(
    const HttpRequestPtr &req,
    std::function<void(std::optional<std::string>)> callback
) {
    auto auth = req->getHeader("Authorization");
    if (auth.empty() || auth.substr(0, 7) != "Bearer ") {
        callback(std::nullopt);
        return;
    }
    std::string token = auth.substr(7);
    auto payload = getTokenContent(token);
    if (!payload || payload->exp < std::chrono::system_clock::time_point()) {
        callback(std::nullopt);
        return;
    }
    auto db = getDbClient();
    db->execSqlAsync(
        R"sql(SELECT token_number FROM users WHERE login = $1)sql",
        [payload, callback](const drogon::orm::Result &r) {
            if (r.empty()) {
                callback(std::nullopt);
                return;
            }
            int dbTokenNumber = r[0]["token_number"].as<int>();
            if (dbTokenNumber != payload->token_number) {
                callback(std::nullopt);
                return;
            }
            callback(payload->login);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << e.base().what();
            callback(std::nullopt);
        },
        payload->login
    );
}