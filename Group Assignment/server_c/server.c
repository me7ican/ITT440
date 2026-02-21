\
#include <arpa/inet.h>
#include <mysql/mysql.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static const char *DB_HOST, *DB_USER, *DB_PASS, *DB_NAME, *SCORE_USER;
static int DB_PORT = 3306;
static int LISTEN_PORT = 5001;
static int UPDATE_SECONDS = 30;

static void die(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

static MYSQL *db_connect(void) {
  MYSQL *conn = mysql_init(NULL);
  if (!conn) {
    fprintf(stderr, "mysql_init failed\n");
    exit(EXIT_FAILURE);
  }
  if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, DB_PORT, NULL, 0)) {
    fprintf(stderr, "mysql_real_connect failed: %s\n", mysql_error(conn));
    exit(EXIT_FAILURE);
  }
  return conn;
}

static void db_upsert_increment(MYSQL *conn) {
  char q[512];
  snprintf(q, sizeof(q),
           "INSERT INTO scoreboard (user, points, datetime_stamp) "
           "VALUES ('%s', 1, NOW()) "
           "ON DUPLICATE KEY UPDATE points = points + 1, datetime_stamp = NOW();",
           SCORE_USER);

  if (mysql_query(conn, q) != 0) {
    fprintf(stderr, "Update query failed: %s\n", mysql_error(conn));
  }
}

static int db_get_latest(MYSQL *conn, int *points_out, char *ts_out, size_t ts_sz) {
  char q[256];
  snprintf(q, sizeof(q),
           "SELECT points, DATE_FORMAT(datetime_stamp,'%%Y-%%m-%%d %%H:%%i:%%s') "
           "FROM scoreboard WHERE user='%s' LIMIT 1;",
           SCORE_USER);

  if (mysql_query(conn, q) != 0) {
    fprintf(stderr, "Select query failed: %s\n", mysql_error(conn));
    return -1;
  }

  MYSQL_RES *res = mysql_store_result(conn);
  if (!res) return -1;

  MYSQL_ROW row = mysql_fetch_row(res);
  if (!row) {
    mysql_free_result(res);
    return -1;
  }

  *points_out = atoi(row[0]);
  snprintf(ts_out, ts_sz, "%s", row[1] ? row[1] : "");
  mysql_free_result(res);
  return 0;
}

static void *updater_thread(void *arg) {
  (void)arg;
  MYSQL *conn = db_connect();
  while (1) {
    db_upsert_increment(conn);
    sleep((unsigned int)UPDATE_SECONDS);
  }
  mysql_close(conn);
  return NULL;
}

static void handle_client(int client_fd) {
  MYSQL *conn = db_connect();

  char buf[128];
  ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
  if (n <= 0) {
    mysql_close(conn);
    return;
  }
  buf[n] = '\0';

  if (strncmp(buf, "GET", 3) == 0) {
    int points = 0;
    char ts[64] = {0};
    if (db_get_latest(conn, &points, ts, sizeof(ts)) == 0) {
      char reply[256];
      snprintf(reply, sizeof(reply),
               "{\"user\":\"%s\",\"points\":%d,\"datetime_stamp\":\"%s\"}\n",
               SCORE_USER, points, ts);
      send(client_fd, reply, strlen(reply), 0);
    } else {
      const char *err = "{\"error\":\"no data\"}\n";
      send(client_fd, err, strlen(err), 0);
    }
  } else {
    const char *msg = "{\"error\":\"use GET\"}\n";
    send(client_fd, msg, strlen(msg), 0);
  }

  mysql_close(conn);
}

int main(void) {
  DB_HOST = getenv("DB_HOST");
  DB_USER = getenv("DB_USER");
  DB_PASS = getenv("DB_PASS");
  DB_NAME = getenv("DB_NAME");
  SCORE_USER = getenv("SCORE_USER");

  const char *dbp = getenv("DB_PORT");
  const char *lp  = getenv("LISTEN_PORT");
  const char *us  = getenv("UPDATE_SECONDS");

  if (dbp) DB_PORT = atoi(dbp);
  if (lp)  LISTEN_PORT = atoi(lp);
  if (us)  UPDATE_SECONDS = atoi(us);

  if (!DB_HOST || !DB_USER || !DB_PASS || !DB_NAME || !SCORE_USER) {
    fprintf(stderr, "Missing env vars\n");
    return 1;
  }

  pthread_t tid;
  if (pthread_create(&tid, NULL, updater_thread, NULL) != 0) die("pthread_create");

  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) die("socket");

  int opt = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons((uint16_t)LISTEN_PORT);

  if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) die("bind");
  if (listen(s, 10) < 0) die("listen");

  printf("C server listening on %d, updating user=%s every %d seconds\n",
         LISTEN_PORT, SCORE_USER, UPDATE_SECONDS);

  while (1) {
    int cfd = accept(s, NULL, NULL);
    if (cfd < 0) continue;
    handle_client(cfd);
    close(cfd);
  }

  close(s);
  return 0;
}
