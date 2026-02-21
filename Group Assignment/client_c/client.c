\
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void die(const char *msg) {
  perror(msg);
  exit(1);
}

int main(void) {
  const char *host = getenv("SERVER_HOST");
  const char *port = getenv("SERVER_PORT");
  if (!host) host = "server_c";
  if (!port) port = "5001";

  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(host, port, &hints, &res) != 0) die("getaddrinfo");

  int s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (s < 0) die("socket");

  if (connect(s, res->ai_addr, res->ai_addrlen) < 0) die("connect");
  freeaddrinfo(res);

  const char *msg = "GET\n";
  send(s, msg, strlen(msg), 0);

  char buf[512];
  ssize_t n = recv(s, buf, sizeof(buf) - 1, 0);
  if (n > 0) {
    buf[n] = '\0';
    printf("Reply: %s", buf);
  }

  close(s);
  return 0;
}
