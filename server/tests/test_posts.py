from conftest import BASE_URL

class TestPosts:
    def test_create_and_get_post(self, auth_session):
        session, user_data = auth_session
        post_data = {
            "content": "Test post content",
            "tags": ["testing", "c++"],
            "img": []
        }
        create_resp = session.post(f"{BASE_URL}/posts/new", json=post_data)
        assert create_resp.status_code == 200
        post_id = create_resp.json()["id"]

        get_resp = session.get(f"{BASE_URL}/posts/{post_id}")
        assert get_resp.status_code == 200
        assert get_resp.json()["content"] == "Test post content"

    def test_post_limits_and_validation(self, auth_session):
        session, _ = auth_session
        # Тест слишком большого количества тегов
        bad_post = {"content": "Hello", "tags": ["tag"] * 21, "img": []}
        resp = session.post(f"{BASE_URL}/posts/new", json=bad_post)
        assert resp.status_code == 400

    def test_likes_dislikes(self, auth_session):
        session, _ = auth_session
        post_id = session.post(f"{BASE_URL}/posts/new", json={"content": "Rate me", "tags": [], "img": []}).json()["id"]
        
        session.post(f"{BASE_URL}/posts/{post_id}/like")
        post = session.get(f"{BASE_URL}/posts/{post_id}").json()
        assert post["likesCount"] == 1
        
        # Меняем лайк на дизлайк
        session.post(f"{BASE_URL}/posts/{post_id}/dislike")
        post = session.get(f"{BASE_URL}/posts/{post_id}").json()
        assert post["likesCount"] == 0
        assert post["dislikesCount"] == 1

    def test_feed_pagination(self, auth_session):
        session, _ = auth_session
        # Создаем 3 поста
        for i in range(3):
            session.post(f"{BASE_URL}/posts/new", json={"content": f"Post {i}", "tags": [], "img": []})
            
        # Запрашиваем с лимитом 2
        feed = session.get(f"{BASE_URL}/posts/feed/me?limit=2").json()
        assert len(feed) == 2
        
        # Получаем курсор из последнего поста
        last_post = feed[-1]
        cursor = f"{last_post['createdAt']}:{last_post['id']}"
        
        # Запрашиваем следующую страницу
        next_page = session.get(f"{BASE_URL}/posts/feed/me?limit=2&cursor={cursor}").json()
        assert len(next_page) == 1 