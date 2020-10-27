# Distroless docker image with ma_echo_server example statically linked with Musl

## Assumptions

1. All commands use Bash syntax
1. Current directory is directory where this repository is cloned
1. Docker 17.06+ for building

## Building image

```bash
docker build -t abrarov/tcp-echo:static docker/ma_echo_server/static
```

## Using image

List content of image:

```bash
temp_dir="$(mktemp -d)" && \
docker save abrarov/tcp-echo:static | tar xf - -C "${temp_dir}" && \
tar tvf "${temp_dir}/"*"/layer.tar" && \
rm -rf "${temp_dir}"
```

Expected output looks like:

```text
drwxr-xr-x 0/0               0 2020-10-27 23:22 opt/
drwxr-xr-x 0/0               0 2020-10-27 23:22 opt/ma_echo_server/
-rwxr-xr-x 0/0         8579200 2020-10-27 23:22 opt/ma_echo_server/ma_echo_server
```

Run ma_echo_server:

```bash
docker run -d --name echo -p 9:9999 abrarov/tcp-echo:static --port 9999 --inactivity-timeout 300
```

Get help about supported parameters:

```bash
docker run --rm abrarov/tcp-echo:static --help
```
