import pytest
import requests
import uuid
import random
import string

BASE_URL = "http://localhost:8080/api"

def generate_random_string(length=10):
    return ''.join(random.choices(string.ascii_letters + string.digits, k=length))

@pytest.fixture
def unique_user_data():
    unique_id = str(uuid.uuid4())[:8]
    return {
        "login": f"user_{unique_id}",
        "email": f"mail_{unique_id}@example.com",
        "password": "StrongPassword123",
        "isPublic": True,
        "phone": f"+{random.randint(10000000000, 99999999999)}"
    }

@pytest.fixture
def auth_session(unique_user_data):
    """Регистрирует пользователя, логинится и возвращает сессию с заголовком Authorization"""
    requests.post(f"{BASE_URL}/auth/register", json=unique_user_data)
    login_resp = requests.post(f"{BASE_URL}/auth/sign-in", json={
        "login": unique_user_data["login"],
        "password": unique_user_data["password"]
    })
    token = login_resp.json()["token"]
    
    session = requests.Session()
    session.headers.update({"Authorization": f"Bearer {token}"})
    return session, unique_user_data

@pytest.fixture
def auth_session_private():
    """Создает приватный аккаунт"""
    unique_id = str(uuid.uuid4())[:8]
    data = {
        "login": f"priv_{unique_id}",
        "email": f"priv_{unique_id}@example.com",
        "password": "StrongPassword123",
        "isPublic": False,
        "phone": f"+{random.randint(10000000000, 99999999999)}"
    }
    requests.post(f"{BASE_URL}/auth/register", json=data)
    token = requests.post(f"{BASE_URL}/auth/sign-in", json={"login": data["login"], "password": data["password"]}).json()["token"]
    session = requests.Session()
    session.headers.update({"Authorization": f"Bearer {token}"})
    return session, data