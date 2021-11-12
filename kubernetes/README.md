# ma_echo_server example in Kubernetes and OpenShift

## Running ma_echo_server example in Kubernetes

All commands were tested using Bash on Ubuntu Server 18.04.

### Assumptions

1. Bash is used for execution of shell commands.
1. Current directory is directory where this repository is cloned.
1. Docker 19.03+
1. Netcat for testing outside Kubernetes.

### kubectl Setup

```bash
k8s_version="1.22.3" && \
curl -Ls "https://storage.googleapis.com/kubernetes-release/release/v${k8s_version}/bin/linux/amd64/kubectl" \
  | sudo tee /usr/local/bin/kubectl >/dev/null && \
sudo chmod +x /usr/local/bin/kubectl
```

### Helm Setup

```bash
helm_version="3.7.1" && \
curl -Ls "https://get.helm.sh/helm-v${helm_version}-linux-amd64.tar.gz" \
  | sudo tar -xz --strip-components=1 -C /usr/local/bin "linux-amd64/helm"
```

### Minikube Setup

In case of need in Kubernetes (K8s) instance one can use [Minikube](https://kubernetes.io/docs/tasks/tools/install-minikube/) to setup local K8s instance easily

1. Download Minikube executable (minikube)

   ```bash
   minikube_version="1.24.0" && \
   curl -Ls "https://github.com/kubernetes/minikube/releases/download/v${minikube_version}/minikube-linux-amd64.tar.gz" \
     | tar -xzO --strip-components=1 "out/minikube-linux-amd64" \
     | sudo tee /usr/local/bin/minikube >/dev/null && \
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

### Deployment into K8s

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

   Expected output looks like:

   ```text
   Release "asio-samples" does not exist. Installing it now.
   NAME: asio-samples
   LAST DEPLOYED: ...
   NAMESPACE: ...
   STATUS: deployed
   REVISION: 1
   NOTES:
   1. Application port: 30007
   ```

1. Run the test for deployed application

   ```bash
   helm test "${helm_release}" -n "${k8s_namespace}" --logs
   ```

   Expected output looks like:

   ```text
   NAME: asio-samples
   LAST DEPLOYED: ...
   NAMESPACE: ...
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
   { echo 'Hello outer World!' | timeout 1s nc "$(minikube ip)" "${port}" || true; }
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

### Minikube Removal

1. Delete minikube instance

   ```bash
   minikube delete --purge=true
   ```

1. Optionally remove K8s configuration files

   ```bash
   rm -rf ~/.kube
   ```

## Running ma_echo_server example in OpenShift

All commands were tested using Bash on CentOS 7.

### Assumptions

1. Bash is used for execution of shell commands.
1. Current directory is directory where this repository is cloned.
1. Docker 1.12+
1. Netcat for testing outside OpenShift.

### oc Client Tools Setup

[oc Client Tools](https://www.okd.io/download.html) can be used to

* Setup local instance of [OKD](https://www.okd.io)
* Communicate with OpenShift cluster (existing OpenShift cluster or local OKD instance)

Setup of oc commandline tool from oc Client Tools can be done using following command

```bash
openshift_version="3.11.0" && openshift_build="0cbc58b" && \
curl -Ls "https://github.com/openshift/origin/releases/download/v${openshift_version}/openshift-origin-client-tools-v${openshift_version}-${openshift_build}-linux-64bit.tar.gz" \
  | sudo tar -xz --strip-components=1 -C /usr/bin "openshift-origin-client-tools-v${openshift_version}-${openshift_build}-linux-64bit/oc"
```

### OKD Setup

In case of need in OpenShift instance one can use [OKD](https://www.okd.io/) to setup local OpenShift instance easily

1. Configure Docker insecure registry - add 172.30.0.0/16 subnet into insecure-registries list of
   Docker daemon configuration, e.g. into /etc/docker/daemon.json file.

   Like this

   ```json
   {
     "insecure-registries": ["172.30.0.0/16"]
   }
   ```

1. Restart Docker daemon to apply changes
1. Determine & decide what address (existing domain name or IP address) will be used to access OpenShift,
   e.g. localhost or IP address of VM.

   Let's assume that OpenShift address is defined in `openshift_address` environment variable, e.g.

   ```bash
   openshift_address="$(ip address show \
     | sed -r 's/^[[:space:]]*inet (192(\.[0-9]{1,3}){3})\/[0-9]+ brd (([0-9]{1,3}\.){3}[0-9]{1,3}) scope global .*$/\1/;t;d' \
     | head -n 1)"
   ``` 

1. Create & start OKD instance

   ```bash
   openshift_version="3.11.0" && \
   openshift_short_version="$(echo ${openshift_version} \
     | sed -r 's/^([0-9]+\.[0-9]+)\.[0-9]+$/\1/')" && \
   docker pull "docker.io/openshift/origin-control-plane:v${openshift_short_version}" && \
   docker pull "docker.io/openshift/origin-hyperkube:v${openshift_short_version}" && \
   docker pull "docker.io/openshift/origin-hypershift:v${openshift_short_version}" && \
   docker pull "docker.io/openshift/origin-node:v${openshift_short_version}" && \
   docker pull "docker.io/openshift/origin-pod:v${openshift_short_version}" && \
   docker pull "docker.io/openshift/origin-deployer:v${openshift_short_version}" && \
   docker pull "docker.io/openshift/origin-cli:v${openshift_short_version}" && \
   docker pull "docker.io/openshift/origin-docker-registry:v${openshift_short_version}" && \
   docker pull "docker.io/openshift/origin-service-serving-cert-signer:v${openshift_short_version}" && \
   oc cluster up \
     --base-dir="${HOME}/openshift.local.clusterup" \
     --public-hostname="${openshift_address}" \
     --enable="registry"
   ```

### Deployment into OpenShift

1. Define OpenShift API server and credentials, namespace, application name and Helm release:

   1. OpenShift API server address (FQDN or IP address) is defined by `openshift_address` environment variable
   1. OpenShift API server user name is defined by `openshift_user` environment variable
   1. OpenShift API server user password is defined by `openshift_password` environment variable
   1. OpenShift registry is defined by `openshift_registry` environment variable
   1. Name of OpenShift namespace for deployment is defined by `openshift_namespace` environment variable
   1. Name of OpenShift application is defined by `openshift_app` environment variable
   1. Name of Helm release is defined by `helm_release` environment variable

   e.g.

   ```bash
   openshift_address="$(ip address show \
     | sed -r 's/^[[:space:]]*inet (192(\.[0-9]{1,3}){3})\/[0-9]+ brd (([0-9]{1,3}\.){3}[0-9]{1,3}) scope global .*$/\1/;t;d' \
     | head -n 1)" && \
   openshift_user="developer" && \
   openshift_password="developer" && \
   openshift_registry="172.30.1.1:5000" && \
   openshift_namespace="myproject" && \
   openshift_app="tcp-echo" && \
   helm_release="asio-samples"
   ```

1. Push abrarov/tcp-echo:latest docker image into OpenShift registry

   ```bash
   docker tag abrarov/tcp-echo "${openshift_registry}/${openshift_namespace}/tcp-echo" && \
   oc login -u "${openshift_user}" -p "${openshift_password}" \
     --insecure-skip-tls-verify=true "${openshift_address}:8443" && \
   docker login -p "$(oc whoami -t)" -u unused "${openshift_registry}" && \
   docker push "${openshift_registry}/${openshift_namespace}/tcp-echo"
    ```

1. Deploy application using [kubernetes/tcp-echo](tcp-echo) Helm chart and wait for completion of rollout

   ```bash
   oc login -u "${openshift_user}" -p "${openshift_password}" \
     --insecure-skip-tls-verify=true "${openshift_address}:8443" && \
   helm upgrade "${helm_release}" kubernetes/tcp-echo \
     --kube-apiserver "https://${openshift_address}:8443" \
     -n "${openshift_namespace}" \
     --set nameOverride="${openshift_app}" \
     --set securityContext.runAsUser=null \
     --set container.image.repository="${openshift_registry}/${openshift_namespace}/tcp-echo" \
     --install --wait
   ```

   Expected output looks like:

   ```text
   Release "asio-samples" does not exist. Installing it now.
   NAME: asio-samples
   LAST DEPLOYED: ...
   NAMESPACE: ...
   STATUS: deployed
   REVISION: 1
   NOTES:
   1. Application port: 30007
   ```

1. Run the test for deployed application

   ```bash
   oc login -u "${openshift_user}" -p "${openshift_password}" \
     --insecure-skip-tls-verify=true "${openshift_address}:8443" >/dev/null && \
   helm test "${helm_release}" \
     --kube-apiserver "https://${openshift_address}:8443" \
     -n "${openshift_namespace}" --logs
   ```

   Expected output looks like:

   ```text
   NAME: asio-samples
   LAST DEPLOYED: ...
   NAMESPACE: ...
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

1. Connect to deployed application outside OpenShift and test echo

   ```bash
   oc login -u "${openshift_user}" -p "${openshift_password}" \
     --insecure-skip-tls-verify=true "${openshift_address}:8443" >/dev/null && \
   port="$(oc get service --insecure-skip-tls-verify=true \
     -s "https://${openshift_address}:8443" -n "${openshift_namespace}" \
     --selector="app.kubernetes.io/instance=${helm_release},app.kubernetes.io/name=${openshift_app}" \
     -o=jsonpath='{.items[0].spec.ports[0].nodePort}')" && \
   { echo 'Hello outer World!' | timeout 1s nc "${openshift_address}" "${port}" || true; }
   ```

   Expected output is:

   ```text
   Hello outer World!
   ```

1. Stop and remove OpenShift application, remove image from OpenShift registry and remove temporary image from local Docker registry

   ```bash
   oc login -u "${openshift_user}" -p "${openshift_password}" \
     --insecure-skip-tls-verify=true "${openshift_address}:8443" && \
   helm uninstall "${helm_release}" \
     --kube-apiserver "https://${openshift_address}:8443" \
     -n "${openshift_namespace}" && \
   oc delete imagestream "${openshift_app}" && \
   docker rmi "${openshift_registry}/${openshift_namespace}/tcp-echo"
   ```

### OKD Removal

1. Stop and remove OKD containers

   ```bash
   oc cluster down
   ```

1. Remove OKD mounts

   ```bash
   for openshift_mount in $(mount | grep openshift | awk '{ print $3 }'); do \
     echo "Unmounting ${openshift_mount}" && sudo umount "${openshift_mount}"; \
   done
   ```

1. Remove OKD configuration

   ```bash
   sudo rm -rf "${HOME}/openshift.local.clusterup"
   ```
