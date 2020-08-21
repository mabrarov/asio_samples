# Docker image building Asio samples on Ubuntu

## Assumptions

1. All commands use Bash syntax
1. Current directory is directory where this repository is cloned

## Building image

```bash
docker build -t abrarov/asio-samples-builder-ubuntu docker/builder/ubuntu
```

## Using image

**Requires** 3.15+ Linux kernel on Docker host in case of building parts which depend on Qt 5.
This requirement comes from Qt 5. Refer to [Fix error while loading shared libraries: libQt5Core.so.5](https://github.com/dnschneid/crouton/wiki/Fix-error-while-loading-shared-libraries:-libQt5Core.so.5)
for explanation.

Refer to [docker/builder/README.md](../README.md)
