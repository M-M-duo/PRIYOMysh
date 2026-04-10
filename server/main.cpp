#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Exception.h>
#include <cstdlib>
#include <string>
#include "controllers/AuthController.h"
#include "helpers.h"

using namespace drogon;

void setupDatabase() {
    auto db = getDbClient();
    if (!db) {
        LOG_FATAL << "Cannot get database client!";
        return;
    }
    LOG_INFO << "Database client obtained successfully";
    db->execSqlAsync(
        R"sql(
        CREATE TABLE IF NOT EXISTS users (
            id SERIAL PRIMARY KEY, 
            login VARCHAR(30) UNIQUE NOT NULL, 
            email VARCHAR(50) UNIQUE NOT NULL, 
            password VARCHAR(250) NOT NULL, 
            is_public BOOLEAN NOT NULL, 
            phone VARCHAR(20) UNIQUE NOT NULL, 
            image TEXT, 
            token_number INTEGER DEFAULT 1, 
            update_token INTEGER DEFAULT 1
        ))sql",
        [](const drogon::orm::Result &) { LOG_INFO << "users table ready"; },
        [](const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << e.base().what();
        }
    );

    db->execSqlAsync(
        R"sql(CREATE EXTENSION IF NOT EXISTS "uuid-ossp")sql",
        [](const drogon::orm::Result &) {
            LOG_INFO << "uuid-ossp extension ready";
        },
        [](const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << e.base().what();
        }
    );

    db->execSqlAsync(
        R"sql(CREATE TABLE IF NOT EXISTS posts (
                 id SERIAL PRIMARY KEY, 
                 id_uuid UUID DEFAULT uuid_generate_v4(), 
                 content VARCHAR(1000) NOT NULL, 
                 author VARCHAR(30) NOT NULL, 
                 created_at TIMESTAMP WITHOUT TIME ZONE))sql",
        [](const drogon::orm::Result &) { LOG_INFO << "posts table ready"; },
        [](const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << e.base().what();
        }
    );

    db->execSqlAsync(
        R"sql(CREATE TABLE IF NOT EXISTS tags (
                     id SERIAL PRIMARY KEY, 
                     id_post INTEGER, 
                     tag VARCHAR(20) NOT NULL))sql",
        [](const drogon::orm::Result &) { LOG_INFO << "tags table ready"; },
        [](const drogon::orm::DrogonDbException &e) {
            LOG_ERROR << e.base().what();
        }
    );

    db->execSqlAsync(
        R"sql(CREATE TABLE IF NOT EXISTS media (
            id SERIAL PRIMARY KEY, 
            id_post INTEGER REFERENCES posts(id) ON DELETE CASCADE, 
            img VARCHAR(200) NOT NULL))sql",
        [](const drogon::orm::Result &) { LOG_INFO << "media table ready"; },
        [](const drogon::orm::DrogonDbException &e) { LOG_ERROR << e.base().what(); }
    );
}

int main() {
    drogon::app().loadConfigFile("../config.json");
    LOG_INFO << "Config loaded";

    auto db = getDbClient();
    if (!db) {
        LOG_FATAL << "Cannot get database client!";
        return 1;
    }
    LOG_INFO << "Database client obtained successfully";

    drogon::app().getLoop()->queueInLoop([]() { setupDatabase(); });

    drogon::app().run();
    return 0;
}