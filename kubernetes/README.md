# Running ma_echo_server example in Kubernetes

All commands were tested using Bash on Ubuntu Server 18.04.

## Assumptions

1. Bash is used for execution of shell commands.
1. Current directory is directory where this repository is cloned.
1. Docker 19.03+

## kubectl Setup

```bash
k8s_version="1.22.3" && \
curl -Ls "https://storage.googleapis.com/kubernetes-release/release/v${k8s_version}/bin/linux/amd64/kubectl" \
  | sudo tee /usr/local/bin/kubectl > /dev/null && \
sudo chmod +x /usr/local/bin/kubectl
```

## Helm Setup

```bash
helm_version="3.7.1" && \
curl -Ls "https://get.helm.sh/helm-v${helm_version}-linux-amd64.tar.gz" \
  | sudo tar -xz --strip-components=1 -C /usr/local/bin "linux-amd64/helm"
```

## Minikube Setup

In case of need in Kubernetes (K8s) instance one can use [Minikube](https://kubernetes.io/docs/tasks/tools/install-minikube/) to setup local K8s instance easily

1. Download Minikube executable (minikube)

   ```bash
   minikube_version="1.24.0" && \
   curl -Ls "https://github.com/kubernetes/minikube/releases/download/v${minikube_version}/minikube-linux-amd64.tar.gz" \
     | tar -xzO --strip-components=1 "out/minikube-linux-amd64" \
     | sudo tee /usr/local/bin/minikube > /dev/null && \
   sudo chmod +x /usr/local/bin/minikube
   ```

1. Create & start K8s instance

   ```bash
   minikube start --driver=docker --addons=registry
   ```

1. Configure Docker insecure registry for Minikube registry - add subnet of Minikube registry into
   insecure-registries list of Docker daemon configuration, e.g. into /etc/docker/daemon.json file.

   Minikube registry IP address can be retrieved using this command

   ```bash
   minikube ip
   ```

   If command returns `192.168.49.2`, then the daemon.json file should look like this

   ```json
   {
     "insecure-registries": ["192.168.49.2/16"]
   }
   ```

1. Stop Minikube

   ```bash
   minikube stop
   ```

1. Restart Docker daemon to apply changes
1. Start Minikube back

   ```bash
   minikube start
   ```

## Deployment into K8s

1. Define K8s namespace, K8s application name and Helm release:

   1. Name of K8s namespace for deployment is defined by `k8s_namespace` environment variable
   1. Name of K8s application is defined by `k8s_app` environment variable
   1. Name of Helm release is defined by `helm_release` environment variable

    e.g.

    ```bash
    k8s_namespace="default" && \
    k8s_app="tcp-echo" && \
    helm_release="asio-samples"
    ```

1. Push abrarov/tcp-echo:latest docker image into Minikube registry

    ```bash
    minikube_registry="$(minikube ip):5000" && \
    docker tag abrarov/tcp-echo:latest "${minikube_registry}/tcp-echo" && \
    docker push "${minikube_registry}/tcp-echo"
    ```

1. Deploy application using [kubernetes/tcp-echo](tcp-echo) Helm chart and wait for completion of rollout

    ```bash
    helm upgrade "${helm_release}" kubernetes/tcp-echo \
     -n "${k8s_namespace}" \
     --set nameOverride="${k8s_app}" \
     --install --wait
    ```

1. Run the test for deployed application

    ```bash
    helm test "${helm_release}" -n "${k8s_namespace}" --logs
    ```

    Expected output looks like:

    ```text
    NAME: asio-samples
    LAST DEPLOYED: ...
    NAMESPACE: default
    STATUS: deployed
    REVISION: 1
    TEST SUITE:     asio-samples-tcp-echo-test
    ...
    Phase:          Succeeded
    NOTES:
    1. Application port: 30007
    
    POD LOGS: asio-samples-tcp-echo-test
    Hello World!
    ```

1. Connect to deployed application outside K8s and test echo

    ```bash
    port="$(kubectl get service -n "${k8s_namespace}" \
      --selector="app.kubernetes.io/instance=${helm_release},app.kubernetes.io/name=${k8s_app}" \
      -o=jsonpath='{.items[0].spec.ports[0].nodePort}')" && \
    echo 'Hello outer World!' | timeout 1s nc "$(minikube ip)" "${port}" || true
    ```
    
    Expected output is:
    
    ```text
    Hello outer World!
    ```

1. Stop and remove K8s application, remove temporary image from local Docker registry

   ```bash
   helm uninstall "${helm_release}" -n "${k8s_namespace}" && \
   minikube_registry="$(minikube ip):5000" && \
   docker rmi "${minikube_registry}/tcp-echo"
   ```

1. Delete minikube instance

   ```bash
   minikube delete --purge=true
   ```

1. Optionally remove K8s configuration files

   ```bash
   rm -rf ~/.kube
   ```
