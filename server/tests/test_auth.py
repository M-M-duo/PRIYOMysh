import pytest
import requests
from conftest import BASE_URL

def test_ping():
    resp = requests.get(f"{BASE_URL}/ping")
    assert resp.status_code == 200

class TestAuth:
    def test_register_success(self, unique_user_data):
        resp = requests.post(f"{BASE_URL}/auth/register", json=unique_user_data)
        print(resp.json())
        assert resp.status_code == 201
        assert resp.json()["profile"]["login"] == unique_user_data["login"]

    def test_register_duplicate_login(self, unique_user_data):
        requests.post(f"{BASE_URL}/auth/register", json=unique_user_data)
        unique_user_data["email"] = "newemail@mail.com"
        unique_user_data["phone"] = "+11111111111"
        resp = requests.post(f"{BASE_URL}/auth/register", json=unique_user_data)
        assert resp.status_code == 409

    @pytest.mark.parametrize("bad_password", ["weak", "NOLOWERCASE1", "nouppercase1", "NoDigitsHere"])
    def test_register_weak_password(self, unique_user_data, bad_password):
        unique_user_data["password"] = bad_password
        resp = requests.post(f"{BASE_URL}/auth/register", json=unique_user_data)
        assert resp.status_code == 400

    def test_login_success(self, unique_user_data):
        requests.post(f"{BASE_URL}/auth/register", json=unique_user_data)
        resp = requests.post(f"{BASE_URL}/auth/sign-in", json={
            "login": unique_user_data["login"],
            "password": unique_user_data["password"]
        })
        assert resp.status_code == 200
        assert "token" in resp.json()

    def test_login_wrong_password(self, unique_user_data):
        requests.post(f"{BASE_URL}/auth/register", json=unique_user_data)
        resp = requests.post(f"{BASE_URL}/auth/sign-in", json={
            "login": unique_user_data["login"],
            "password": "WrongPassword123"
        })
        assert resp.status_code == 401