FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends     build-essential ca-certificates  && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY client.c ./
RUN gcc -O2 -Wall -o client client.c

CMD ["sh", "-c", "sleep 3 && ./client"]
