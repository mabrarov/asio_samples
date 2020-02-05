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
container_id="$(docker create abrarov/tcp-echo:static sh)" \
  && docker export "${container_id}" | tar tf - --verbose && \
  docker rm -fv "${container_id}" > /dev/null
```

Expected output looks like:

```text
-rwxr-xr-x 0/0               0 2020-02-03 02:27 .dockerenv
drwxr-xr-x 0/0               0 2020-02-03 02:27 dev/
-rwxr-xr-x 0/0               0 2020-02-03 02:27 dev/console
drwxr-xr-x 0/0               0 2020-02-03 02:27 dev/pts/
drwxr-xr-x 0/0               0 2020-02-03 02:27 dev/shm/
drwxr-xr-x 0/0               0 2020-02-03 02:27 etc/
-rwxr-xr-x 0/0               0 2020-02-03 02:27 etc/hostname
-rwxr-xr-x 0/0               0 2020-02-03 02:27 etc/hosts
lrwxrwxrwx 0/0               0 2020-02-03 02:27 etc/mtab -> /proc/mounts
-rwxr-xr-x 0/0               0 2020-02-03 02:27 etc/resolv.conf
drwxr-xr-x 0/0               0 2020-02-03 02:19 opt/
drwxr-xr-x 0/0               0 2020-02-03 02:19 opt/ma_echo_server/
-rwxr-xr-x 0/0         8606944 2020-02-03 02:19 opt/ma_echo_server/ma_echo_server
drwxr-xr-x 0/0               0 2020-02-03 02:27 proc/
drwxr-xr-x 0/0               0 2020-02-03 02:27 sys/
```

Run ma_echo_server:

```bash
docker run -d --name echo -p 9:9999 abrarov/tcp-echo:static --port 9999 --inactivity-timeout 300
```

Get help about supported parameters:

```bash
docker run --rm abrarov/tcp-echo:static --help
```
