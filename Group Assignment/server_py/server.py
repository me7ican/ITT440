import os
import socket
import threading
import time

import mysql.connector

DB_HOST = os.getenv("DB_HOST", "db")
DB_PORT = int(os.getenv("DB_PORT", "3306"))
DB_USER = os.getenv("DB_USER", "root")
DB_PASS = os.getenv("DB_PASS", "rootpass")
DB_NAME = os.getenv("DB_NAME", "itt440")

SCORE_USER = os.getenv("SCORE_USER", "py_user")
LISTEN_PORT = int(os.getenv("LISTEN_PORT", "5002"))
UPDATE_SECONDS = int(os.getenv("UPDATE_SECONDS", "30"))


def db_conn():
    return mysql.connector.connect(
        host=DB_HOST,
        port=DB_PORT,
        user=DB_USER,
        password=DB_PASS,
        database=DB_NAME,
    )


def upsert_increment():
    conn = db_conn()
    cur = conn.cursor()
    q = (
        "INSERT INTO scoreboard (user, points, datetime_stamp) "
        "VALUES (%s, 1, NOW()) "
        "ON DUPLICATE KEY UPDATE points = points + 1, datetime_stamp = NOW()"
    )
    cur.execute(q, (SCORE_USER,))
    conn.commit()
    cur.close()
    conn.close()


def get_latest():
    conn = db_conn()
    cur = conn.cursor()
    cur.execute(
        "SELECT points, DATE_FORMAT(datetime_stamp,'%Y-%m-%d %H:%i:%s') "
        "FROM scoreboard WHERE user=%s LIMIT 1",
        (SCORE_USER,),
    )
    row = cur.fetchone()
    cur.close()
    conn.close()
    if not row:
        return None
    return int(row[0]), row[1]


def updater_loop():
    while True:
        upsert_increment()
        time.sleep(UPDATE_SECONDS)


def handle_client(csock: socket.socket):
    try:
        data = csock.recv(128).decode(errors="ignore").strip()
        if data.startswith("GET"):
            latest = get_latest()
            if latest:
                points, ts = latest
                reply = (
                    f'{{"user":"{SCORE_USER}","points":{points},"datetime_stamp":"{ts}"}}\n'
                )
            else:
                reply = '{"error":"no data"}\n'
        else:
            reply = '{"error":"use GET"}\n'
        csock.sendall(reply.encode())
    finally:
        csock.close()


def main():
    threading.Thread(target=updater_loop, daemon=True).start()

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(("0.0.0.0", LISTEN_PORT))
    s.listen(20)

    print(
        f"Python server listening on {LISTEN_PORT}, updating user={SCORE_USER} every {UPDATE_SECONDS}s"
    )

    while True:
        csock, _ = s.accept()
        threading.Thread(target=handle_client, args=(csock,), daemon=True).start()


if __name__ == "__main__":
    main()
