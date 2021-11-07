# Docker image with ma_echo_server example on Windows Nano Server

## Assumptions

1. All commands use Powershell syntax
1. Current directory is directory where this repository is cloned
1. [Docker](https://docs.microsoft.com/en-us/virtualization/windowscontainers/deploy-containers/deploy-containers-on-server) runs on [Windows Server 2019 1809](https://docs.microsoft.com/en-us/windows-server/get-started/windows-server-release-info)
1. Docker 19.03+

## Building image

```powershell
docker build -t abrarov/tcp-echo:windows docker\ma_echo_server\nanoserver
```

## Testing built image

```powershell
($port=9999) -and `
($container_id="$(docker run -d abrarov/tcp-echo:windows `
  --port "${port}" --inactivity-timeout 60)") -and `
(docker stop -t 10 "${container_id}") -and `
((docker inspect --format='{{.State.ExitCode}}' "${container_id}" | Out-Host) -or $true) -and `
(docker rm -fv "${container_id}") | Out-Null
```

Expected output:

```text
0
```

## Using image

Run ma_echo_server:

```powershell
docker run -d --name echo -p 9:9999 abrarov/tcp-echo:windows --port 9999 --inactivity-timeout 300
```

Get help about supported parameters:

```powershell
docker run --rm abrarov/tcp-echo:windows --help
```
