# Distroless docker image with ma_echo_server example statically linked with Musl

## Assumptions

1. All commands use Bash syntax
1. Current directory is directory where this repository is cloned
1. Docker 17.06+ for building

## Building image

```bash
docker build -t abrarov/tcp-echo:static docker/ma_echo_server/static
```

## Testing built image

```bash
port=9999 && \
container_id="$(docker run -d abrarov/tcp-echo:static \
  --port "${port}" --inactivity-timeout 60)" && \
docker run --rm --link "${container_id}:echo" curlimages/curl \
  sh -c "echo 'Hello World"'!'"' | timeout 1s curl -sf \"telnet://echo:${port}\" || true" && \
docker stop -t 10 "${container_id}" >/dev/null && \
docker inspect --format='{{.State.ExitCode}}' "${container_id}" && \
docker rm -fv "${container_id}" >/dev/null
```

Expected output:

```text
Hello World!
Terminated
0
```

## Using image

Run ma_echo_server:

```bash
docker run -d --name echo -p 9:9999 abrarov/tcp-echo:static --port 9999 --inactivity-timeout 300
```

Get help about supported parameters:

```bash
docker run --rm abrarov/tcp-echo:static --help
```
