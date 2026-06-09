#include "../helpers.hpp"
#include <chrono>
#include <drogon/drogon.h>
#include <drogon/drogon_test.h>
#include <filesystem>
#include <fstream>
#include <string>

using namespace drogon;

// --- ТЕСТЫ ВАЛИДАЦИИ ЛОГИНА ---
DROGON_TEST(Validation_Login) {
    // Идеальные случаи
    CHECK(validateLogin("ValidLogin123") == true);
    CHECK(validateLogin("login_with_dashes") == true);
    CHECK(validateLogin("A") == true); // Минимальная длина

    // Крайние случаи по длине (максимум 30)
    std::string exact30(30, 'a');
    std::string over30(31, 'a');
    CHECK(validateLogin(exact30) == true);
    CHECK(validateLogin(over30) == false);
    CHECK(validateLogin("") == false);

    // Недопустимые символы
    CHECK(validateLogin("invalid_login!") == false);
    CHECK(validateLogin("space inside") == false);
    CHECK(validateLogin("русскийТекст") == false);
    CHECK(validateLogin("login@mail") == false);
}

// --- ТЕСТЫ ВАЛИДАЦИИ EMAIL ---
DROGON_TEST(Validation_Email) {
    // Валидные email
    CHECK(validateEmail("test@example.com") == true);
    CHECK(validateEmail("user.name+tag@domain.co.uk") == true);

    // Крайние случаи по длине (максимум 50)
    std::string longPrefix(38, 'a');
    CHECK(validateEmail(longPrefix + "@example.com") == true);   // 50 символов
    CHECK(validateEmail(longPrefix + "a@example.com") == false); // 51 символ

    // Невалидные форматы
    CHECK(validateEmail("plainaddress") == false);
    CHECK(validateEmail("@missinguser.com") == false);
    CHECK(validateEmail("user@.com") == false);
    CHECK(validateEmail("user@domain") == false);   // Нет доменной зоны
    CHECK(validateEmail("user@domain.c") == false); // Зона меньше 2 символов
    CHECK(validateEmail("") == false);
}

// --- ТЕСТЫ ВАЛИДАЦИИ ПАРОЛЯ ---
DROGON_TEST(Validation_PasswordStrength) {
    CHECK(validatePasswordStrength("StrongPass1") == true);

    CHECK(validatePasswordStrength("A1b2c") == false);
    CHECK(validatePasswordStrength("A1b2c3") == true);

    std::string exact100 = "A1b" + std::string(97, 'c');
    std::string over100 = exact100 + "c";
    CHECK(validatePasswordStrength(exact100) == true);
    CHECK(validatePasswordStrength(over100) == false);

    // Проверка наличия обязательных символов
    CHECK(validatePasswordStrength("nouppercase123") == false);
    CHECK(validatePasswordStrength("NOLOWERCASE123") == false);
    CHECK(validatePasswordStrength("NoDigitsHere") == false);
    CHECK(validatePasswordStrength("!@#$%^&*()") == false);
}

// --- ТЕСТЫ ВАЛИДАЦИИ ТЕЛЕФОНА ---
DROGON_TEST(Validation_Phone) {
    CHECK(validatePhone("+12345678901") == true);
    CHECK(validatePhone("+1") == true);

    CHECK(validatePhone("89001234567") == false);
    CHECK(validatePhone("+12345abcde") == false);
    CHECK(validatePhone("++1234567") == false);

    std::string exact20 = "+" + std::string(19, '1');
    std::string over20 = exact20 + "1";
    CHECK(validatePhone(exact20) == true);
    CHECK(validatePhone(over20) == false);
}

// --- ТЕСТЫ КРИПТОГРАФИИ ---
DROGON_TEST(Crypto_HashingAndChecking) {
    std::string password = "SuperSecretPassword2026!";
    std::string hash1 = hashPassword(password);
    std::string hash2 = hashPassword(password);

    CHECK(!hash1.empty());
    CHECK(hash1 != hash2);

    CHECK(checkPassword(password, hash1) == true);
    CHECK(checkPassword(password, hash2) == true);

    // Проверка неверных паролей
    CHECK(checkPassword("WrongPassword", hash1) == false);
    CHECK(checkPassword("", hash1) == false);
    CHECK(checkPassword(password + " ", hash1) == false);
}

// --- СТРЕСС-ТЕСТ ХЕШИРОВАНИЯ (TL) ---
DROGON_TEST(Crypto_Performance) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10; ++i) {
        hashPassword("PerformanceTest123");
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    CHECK(duration < 2000);
}

// --- ТЕСТЫ JWT ТОКЕНОВ ---
DROGON_TEST(JWT_Lifecycle) {
    int userId = 1337;
    int tokenNumber = 5;
    int updateToken = 2;

    std::string token = createToken(userId, tokenNumber, updateToken);
    CHECK(!token.empty());

    auto payload = getTokenContent(token);
    CHECK(payload.has_value() == true);
    if (payload) {
        CHECK(payload->user_id == std::to_string(userId));
        CHECK(payload->token_number == tokenNumber);
        CHECK(payload->update_token == updateToken);
        CHECK(payload->exp > std::chrono::system_clock::now());
    }

    // Тест поврежденного токена
    std::string tamperedToken = token + "bad_data";
    auto badPayload = getTokenContent(tamperedToken);
    CHECK(badPayload.has_value() == false);

    // Тест пустой строки
    auto emptyPayload = getTokenContent("");
    CHECK(emptyPayload.has_value() == false);
}

// --- ТЕСТЫ РАБОТЫ С ИЗОБРАЖЕНИЯМИ (Base64) ---
DROGON_TEST(FileSystem_Base64_Helpers) {
    std::string testDir = "./test_media/";
    if (!std::filesystem::exists(testDir)) {
        std::filesystem::create_directory(testDir);
    }

    std::string originalText = "Fake image binary data for testing purposes.";
    std::string base64Data = drogon::utils::base64Encode(originalText);
    std::string filename = generateFilename(".txt");
    std::string filePath = testDir + filename;

    CHECK(filename.find(".txt") != std::string::npos);
    CHECK(filename.length() > 10);

    // Проверяем сохранение
    bool saved = saveBase64(base64Data, filePath);
    CHECK(saved == true);
    CHECK(std::filesystem::exists(filePath) == true);

    // Проверяем загрузку
    std::string loadedBase64 = loadImageAsBase64(filePath);
    CHECK(loadedBase64 == base64Data);

    // Проверяем обработку ошибок
    CHECK(saveBase64("invalid_base64!!!", testDir + "fail.jpg") == false);
    CHECK(loadImageAsBase64("non_existent_file.jpg") == "");

    std::filesystem::remove_all(testDir);
}

int main(int argc, char **argv) {
    drogon::app().loadConfigFile("../config.json");

    int status = drogon::test::run(argc, argv);

    return status;
}