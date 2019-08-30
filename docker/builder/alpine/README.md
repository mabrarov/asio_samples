# Docker image building Asio samples on Alpine Linux

## Assumptions

1. All commands use Bash syntax
1. Current directory is directory where this repository is cloned

## Building image

```bash
docker build -t abrarov/asio-samples-builder-alpine docker/builder/alpine
```

## Using image

Refer to [docker/builder/README.md](../README.md)
