#include <drogon/drogon.h>
#include <cstdlib>
#include <string>
#include "controllers/AuthController.h"
#include "helpers.h"

using namespace drogon;

void setupDatabase() {
    auto db = getDbClient();
    db->execSqlAsync("CREATE TABLE IF NOT EXISTS users ("
                     "id SERIAL PRIMARY KEY, "
                     "login VARCHAR(30) UNIQUE NOT NULL, "
                     "email VARCHAR(50) UNIQUE NOT NULL, "
                     "password VARCHAR(250) NOT NULL, "
                     "is_public BOOLEAN NOT NULL, "
                     "phone VARCHAR(20), "
                     "image TEXT, "
                     "token_number INTEGER DEFAULT 1, "
                     "update_token INTEGER DEFAULT 1)",
                     [](const drogon::orm::Result&) { LOG_INFO << "users table ready"; },
                     [](const drogon::orm::DrogonDbException& e) { LOG_ERROR << e.base().what(); });
}

int main() {
    auto postgres_host = std::getenv("POSTGRES_HOST");
    auto postgres_port = std::getenv("POSTGRES_PORT");
    auto postgres_db   = std::getenv("POSTGRES_DATABASE");
    auto postgres_user = std::getenv("POSTGRES_USERNAME");
    auto postgres_pass = std::getenv("POSTGRES_PASSWORD");
    auto secret        = std::getenv("RANDOM_SECRET");
    auto server_port   = std::getenv("SERVER_PORT");

    app().loadConfigFile("config.json");

    setupDatabase();

    LOG_INFO << "Server running on port " << (server_port ? server_port : "8080");
    app().run();
    return 0;
}