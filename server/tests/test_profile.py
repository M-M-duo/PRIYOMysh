import requests
from conftest import BASE_URL

class TestProfile:
    def test_get_my_profile(self, auth_session):
        session, user_data = auth_session
        resp = session.get(f"{BASE_URL}/profiles/me")
        assert resp.status_code == 200
        assert resp.json()["login"] == user_data["login"]
        assert resp.json()["isMe"] == True

    def test_update_profile_success(self, auth_session):
        session, _ = auth_session
        new_data = {
            "isPublic": False,
            "phone": "+99999999999"
        }
        resp = session.patch(f"{BASE_URL}/me/profile", json=new_data)
        assert resp.status_code == 200
        assert resp.json()["isPublic"] == False
        assert resp.json()["phone"] == "+99999999999"

    def test_update_password(self, auth_session):
        session, user_data = auth_session
        resp = session.post(f"{BASE_URL}/me/updatePassword", json={
            "oldPassword": user_data["password"],
            "newPassword": "NewStrongPassword123"
        })
        assert resp.status_code == 200
        
        # Проверяем, что старый пароль больше не работает
        login_resp = requests.post(f"{BASE_URL}/auth/sign-in", json={
            "login": user_data["login"],
            "password": user_data["password"]
        })
        assert login_resp.status_code == 401

    def test_get_other_profile_private(self, auth_session, auth_session_private):
        # Первый пользователь пытается посмотреть профиль приватного пользователя
        session_1, _ = auth_session
        session_private, user_private = auth_session_private
        
        # Получаем UUID приватного пользователя
        profile_resp = session_private.get(f"{BASE_URL}/profiles/me")
        private_uuid = profile_resp.json()["id"]

        # Пытаемся получить его профиль первым пользователем (не друзья)
        resp = session_1.get(f"{BASE_URL}/profiles/{private_uuid}")
        assert resp.status_code == 200
        assert resp.json()["allowedToSee"] == False