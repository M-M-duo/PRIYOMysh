from conftest import BASE_URL

class TestFriends:
    def test_add_and_remove_friend(self, auth_session, auth_session_private):
        session_1, _ = auth_session
        session_2, _ = auth_session_private
        
        uuid_2 = session_2.get(f"{BASE_URL}/profiles/me").json()["id"]
        
        # 1 добавляет 2 в друзья
        add_resp = session_1.post(f"{BASE_URL}/friends/add", json={"id": uuid_2})
        assert add_resp.status_code == 200
        
        # Проверяем список following у 1
        following = session_1.get(f"{BASE_URL}/friends/me/following").json()
        assert any(f["id"] == uuid_2 for f in following)
        
        # 1 удаляет 2 из друзей
        rm_resp = session_1.post(f"{BASE_URL}/friends/remove", json={"id": uuid_2})
        assert rm_resp.status_code == 200

    def test_add_self_as_friend_fails(self, auth_session):
        session, _ = auth_session
        my_uuid = session.get(f"{BASE_URL}/profiles/me").json()["id"]
        
        resp = session.post(f"{BASE_URL}/friends/add", json={"id": my_uuid})
        assert resp.status_code == 400
        assert "Cannot add yourself" in resp.json()["reason"]