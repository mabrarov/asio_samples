# Docker image with ma_echo_server example on Google distroless image

## Assumptions

1. All commands use Bash syntax
1. Current directory is directory where this repository is cloned
1. Docker 17.06+ for building

## Building image

```bash
docker build -t abrarov/tcp-echo:distroless docker/ma_echo_server/distroless
```

## Using image

Run ma_echo_server:

```bash
docker run -d --name echo -p 9:9999 abrarov/tcp-echo:distroless --port 9999 --inactivity-timeout 300
```

Get help about supported parameters:

```bash
docker run --rm abrarov/tcp-echo:distroless --help
```
