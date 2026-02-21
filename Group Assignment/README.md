# ITT440 Group Assignment (Socket Programming + Docker + MySQL)

**Group:** Amir & Sabree

This project runs:
- 1x MySQL DB container (table: `scoreboard(user, points, datetime_stamp)`)
- 2x Server containers:
  - **C Server** (`server_c`) updates user `c_user` every 30 seconds, listens on port **5001**
  - **Python Server** (`server_py`) updates user `py_user` every 30 seconds, listens on port **5002**
- 2x Client containers:
  - **C Client** (`client_c`) connects ONLY to `server_c`
  - **Python Client** (`client_py`) connects ONLY to `server_py`

Clients send `GET` to their server.
Servers read the latest row from MySQL and reply with a JSON line.

## Requirements
- Docker + Docker Compose

## How to run
From the project root:

```bash
docker compose build
docker compose up
```

## Expected output
You will see:
- C server and Python server startup logs
- Both clients printing a JSON reply like:

```json
{"user":"c_user","points":3,"datetime_stamp":"2026-02-22 12:34:56"}
```

## Verify the DB updates
Run:

```bash
docker exec -it itt440_db mysql -uroot -prootpass -e "SELECT * FROM itt440.scoreboard;"
```

You should see both users and points increasing over time.

## Ports
- C server: `localhost:5001`
- Python server: `localhost:5002`
- MySQL: `localhost:3306`

> Note: If your lecturer wants different port numbers, change them in `docker-compose.yml`.
