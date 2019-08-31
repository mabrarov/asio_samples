# Docker image building Asio samples on Ubuntu

## Assumptions

1. All commands use Bash syntax
1. Current directory is directory where this repository is cloned

## Building image

```bash
docker build -t abrarov/asio-samples-builder-ubuntu docker/builder/ubuntu
```

## Using image

Refer to [docker/builder/README.md](../README.md)
