# Docker image with ma_echo_server example on Alpine Linux

## Assumptions

1. All commands use Bash syntax
1. Current directory is directory where this repository is cloned

## Building image

```bash
docker build -t abrarov/tcp-echo docker/ma_echo_server/alpine
```

## Using image

Run ma_echo_server:

```bash
docker run -d --name echo -p 9:9999 abrarov/tcp-echo --port 9999 --inactivity-timeout 300
```

Get help about supported parameters:

```bash
docker run --rm abrarov/tcp-echo --help
```
