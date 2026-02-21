FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends     build-essential pkg-config default-libmysqlclient-dev ca-certificates  && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY server.c Makefile ./
RUN make

EXPOSE 5001
CMD ["./server"]
