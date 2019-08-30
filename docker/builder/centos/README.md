# Docker image building Asio samples on CentOS

## Assumptions

1. All commands use Bash syntax
1. Current directory is directory where this repository is cloned

## Building image

```bash
docker build -t abrarov/asio-samples-builder-centos docker/builder/centos
```

## Using image

Refer to [docker/builder/README.md](../README.md)
