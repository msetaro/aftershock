FROM debian:bookworm-slim AS engine
RUN apt-get update && apt-get install -y --no-install-recommends g++ cmake ninja-build python3 ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /source
COPY CMakeLists.txt COPYING.txt ./
COPY cmake/ cmake/
COPY engine/ engine/
COPY game/ game/
COPY third_party/ third_party/
COPY tools/level/ tools/level/
COPY tools/match/content.py tools/match/content.py
COPY tests/assets/levels/ tests/assets/levels/
COPY tests/golden/levels/ tests/golden/levels/
RUN cmake -S . -B /build -G Ninja -DBUILD_CLIENT=OFF -DUSE_SDL=OFF -DUSE_CURL=OFF -DUSE_CCACHE=OFF && cmake --build /build -j 4 && cp /build/release-linux-*/quake3e.ded.* /server
RUN python3 tools/match/content.py /content

FROM golang:1.27.1-bookworm AS controller
WORKDIR /source
COPY tools/match/go.mod tools/match/go.sum ./
RUN go mod download
COPY tools/match/*.go ./
RUN CGO_ENABLED=0 go build -trimpath -ldflags='-s -w' -o /match . && mkdir /empty && touch /empty/.keep

FROM gcr.io/distroless/cc-debian12:nonroot
COPY --from=engine /server /app/server
COPY --from=engine /content /content
COPY --from=controller /match /app/match
COPY --from=controller --chown=65532:65532 /empty /home/match
COPY --from=controller --chown=65532:65532 /empty /state
USER 65532:65532
WORKDIR /home/match
EXPOSE 27960/udp
ENTRYPOINT ["/app/match"]
CMD ["run"]
