# ПРИЁМышь (PRIYOMysh) - десктопная социальная сеть

## Установка и запуск 
[Client setup instructions](https://github.com/M-M-duo/PRIYOMysh/tree/main/client/README.md) <br />
[Server setup instructions](https://github.com/M-M-duo/PRIYOMysh/tree/main/server/README.md)

## Функционал

- Регистрация и аутентификация
- Редактирование собственного профиля
- Создание постов (текст, теги, изображения)
- Ленты: свои посты, посты другого пользователя, новостная лента (публичные + подписчики), лента друзей (свои + взаимные подписки)
- Система подписок и подписчиков
- Лайки / дизлайки постов
- Поиск пользователей по логину
- Курсорная пагинация

## Архитектура проекта

```mermaid
graph LR
    Клиент[Qt Client] --> REST[Drogon API]
    REST --> PostgreSQL
    REST --> FS[Файлы ./media]
```

## Технологии

### Сервер (`/server`)

* **Язык:** C++17
* **Фреймворк:** Drogon Framework
* **База данных:** PostgreSQL 14+ (с использованием расширения `uuid-ossp`)
* **Безопасность:** Авторизация на базе JWT
* **Тестирование:** Drogon Test (unit-тесты) / PyTest (E2E API тесты)

### Клиент (`/client`)
Графическое приложение со строгим разделением логики.
* **Язык:** C++17
* **Фреймворк:** Qt6 (QtCore, QtGui, QtWidgets, QtNetwork, QImage)
* **Тестирование:** Ctest


## Скриншоты интерфейса

<!-- Вставьте реальные скриншоты, заменив пути -->
1. Вход / регистрация  
   <img src="screenshots/login.png" width="300"> <img src="screenshots/registration.png" width="300"> <img src="screenshots/auth.png" width="300">
3. Лента  
   <img src="screenshots/feed.png" width="300"> <img src="screenshots/friends_feed.png" width="300">
4. Создание поста  
   <img src="screenshots/new_post.png" width="300">
5. Профиль пользователя  
   <img src="screenshots/profile.png" width="300"> <img src="screenshots/empty_profile.png" width="300">
6. Редактирование профиля  
   <img src="screenshots/edit_profile.png" width="300">
7. Друзья / подписчики  
   <img src="screenshots/friends.png" width="300">
   
## Команда разработчиков:
<a href="https://github.com/M-M-duo/PRIYOMysh/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=M-M-duo/PRIYOMysh" />
</a>

* Вячеслав Возяков – клиент
* Мария Димитриева – сервер + БД
