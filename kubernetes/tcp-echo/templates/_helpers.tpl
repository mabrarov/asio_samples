{{/*
Expand the name of the chart.
*/}}
{{- define "tcp-echo.name" -}}
{{- .Values.nameOverride | default .Chart.Name | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Create a default fully qualified app name.
We truncate at 63 chars because some Kubernetes name fields are limited to this (by the DNS naming spec).
If release name contains chart name it will be used as a full name.
*/}}
{{- define "tcp-echo.fullname" -}}
{{- if .Values.fullnameOverride }}
{{- .Values.fullnameOverride | trunc 63 | trimSuffix "-" }}
{{- else }}
{{- $name := .Values.nameOverride | default .Chart.Name }}
{{- if contains $name .Release.Name }}
{{- .Release.Name | trunc 63 | trimSuffix "-" }}
{{- else }}
{{- printf "%s-%s" .Release.Name $name | trunc 63 | trimSuffix "-" }}
{{- end }}
{{- end }}
{{- end }}

{{/*
Create chart name and version as used by the chart label.
*/}}
{{- define "tcp-echo.chart" -}}
{{- printf "%s-%s" .Chart.Name .Chart.Version | replace "+" "_" | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Selector labels
*/}}
{{- define "tcp-echo.matchLabels" -}}
app.kubernetes.io/name: {{ include "tcp-echo.name" . | quote }}
app.kubernetes.io/instance: {{ .Release.Name | quote }}
{{- end }}

{{/*
Common labels
*/}}
{{- define "tcp-echo.labels" -}}
helm.sh/chart: {{ include "tcp-echo.chart" . | quote }}
{{ include "tcp-echo.matchLabels" . }}
{{- if .Chart.AppVersion }}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
{{- end }}
app.kubernetes.io/managed-by: {{ .Release.Service | quote }}
app: {{ .Release.Name | quote }}
{{- end }}

{{/*
Component labels
*/}}
{{- define "tcp-echo.componentLabels" -}}
app.kubernetes.io/component: "tcp-echo"
{{- end }}

{{/*
Name of deployment.
*/}}
{{- define "tcp-echo.deploymentName" -}}
{{ include "tcp-echo.fullname" . }}
{{- end }}

{{/*
Name of image pull secret.
*/}}
{{- define "tcp-echo.imagePullSecretName" -}}
{{ include "tcp-echo.fullname" . }}
{{- end }}

{{/*
Name of service.
*/}}
{{- define "tcp-echo.serviceName" -}}
{{ include "tcp-echo.fullname" . }}
{{- end }}

{{/*
Docker authentication config for image registry.
{{ include "tcp-echo.dockerRegistryAuthenticationConfig" (dict "imageRegistry" .Values.image.registry "credentials" .Values.image.pullSecret) }}
*/}}
{{- define "tcp-echo.dockerRegistryAuthenticationConfig" -}}
{{- $registry := .imageRegistry }}
{{- $username := .credentials.username }}
{{- $password := .credentials.password }}
{{- $email := .credentials.email }}
{{- printf "{\"auths\":{\"%s\":{\"username\":\"%s\",\"password\":\"%s\",\"email\":\"%s\",\"auth\":\"%s\"}}}" $registry $username $password $email (printf "%s:%s" $username $password | b64enc) | b64enc }}
{{- end }}

{{/*
Name of container port.
*/}}
{{- define "tcp-echo.containerPortName" -}}
tcp-echo
{{- end }}

{{/*
Name of service port.
*/}}
{{- define "tcp-echo.servicePortName" -}}
tcp-echo
{{- end }}

{{/*
Container image full name.
*/}}
{{- define "tcp-echo.imageFullName" -}}
{{ printf "%s/%s:%s" .Values.image.registry .Values.image.repository (.Values.image.tag | default .Chart.AppVersion) }}
{{- end }}

{{/*
Termination grace period.
*/}}
{{- define "tcp-echo.terminationGracePeriod" -}}
{{ add (.Values.tcpEcho.stopTimeout | default 60) 5 }}
{{- end }}

{{/*
Test component labels
*/}}
{{- define "tcp-echo.test.componentLabels" -}}
app.kubernetes.io/component: "test"
{{- end }}

{{/*
Name of test pod.
*/}}
{{- define "tcp-echo.test.podName" -}}
{{ include "tcp-echo.fullname" . }}-test
{{- end }}

{{/*
Name of test image pull secret.
*/}}
{{- define "tcp-echo.test.imagePullSecretName" -}}
{{ include "tcp-echo.fullname" . }}-test
{{- end }}

{{/*
Test container image full name.
*/}}
{{- define "tcp-echo.test.imageFullName" -}}
{{ printf "%s/%s:%s" .Values.test.image.registry .Values.test.image.repository (.Values.test.image.tag | default "latest") }}
{{- end }}

{{/*
Renders a value that contains template.
Usage:
{{ include "tcp-echo.tplValuesRender" (dict "value" .Values.path.to.the.Value "context" $) }}
*/}}
{{- define "tcp-echo.tplValuesRender" -}}
{{- if typeIs "string" .value }}
{{- tpl .value .context }}
{{- else }}
{{- tpl (.value | toYaml) .context }}
{{- end }}
{{- end -}}
