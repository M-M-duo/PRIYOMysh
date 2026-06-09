
# Server (Drogon Framework)

### How to start it up (on Ubuntu / macOS):
1. Clone the repo:
```bash
git clone git@github.com:M-M-duo/PRIYOMysh.git

```

2. Reach the server directory:

```bash
cd server

```

3. Create and configure your environment variables:

```bash
cp .env.example .env
# Edit .env with your database credentials and secret keys

```

4. Install all system dependencies, Drogon Framework, and header libraries:

```bash
chmod +x install_dependencies.sh && ./install_dependencies.sh

```

5. [Optional] Initialize the database schema manually (REQUIRED before running tests):

```bash
# For local PostgreSQL
psql -h 127.0.0.1 -U your_user -d priyomysh_db -f tests/init.sql

```

6. Build and start the server application:

```bash
chmod +x run.sh && ./run.sh

```

### Advanced Run Flags:

* `./run.sh` — to build and run app only
* `./run.sh -t` — to build and run app and tests 
* `./run.sh -nb` — to run pre-built app
