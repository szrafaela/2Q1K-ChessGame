# syntax=docker/dockerfile:1.4

FROM --platform=$BUILDPLATFORM debian:bookworm-slim AS builder
ARG TARGETPLATFORM TARGETARCH TARGETOS
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build ca-certificates && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -S . -B /tmp/build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /tmp/build --config Release

FROM debian:bookworm-slim AS runtime
RUN apt-get update && \
    apt-get install -y --no-install-recommends libstdc++6 stockfish && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /data
COPY --from=builder /tmp/build/chess_app /usr/local/bin/chess_app

# Save files will be written to /data; mount a volume here if you want to persist games.
ENTRYPOINT ["/usr/local/bin/chess_app"]
