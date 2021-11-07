{{/*
Expand the name of the chart.
*/}}
{{- define "app.name" -}}
{{- .Values.nameOverride | default .Chart.Name | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Create a default fully qualified app name.
We truncate at 63 chars because some Kubernetes name fields are limited to this (by the DNS naming spec).
If release name contains chart name it will be used as a full name.
*/}}
{{- define "app.fullname" -}}
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
{{- define "app.chart" -}}
{{- printf "%s-%s" .Chart.Name .Chart.Version | replace "+" "_" | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Common labels
*/}}
{{- define "app.labels" -}}
helm.sh/chart: {{ include "app.chart" . | quote }}
{{ include "app.selectorLabels" . }}
{{- if .Chart.AppVersion }}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
{{- end }}
app.kubernetes.io/managed-by: {{ .Release.Service | quote }}
app: {{ include "app.name" . | quote }}
{{- end }}

{{/*
Selector labels
*/}}
{{- define "app.selectorLabels" -}}
app.kubernetes.io/name: {{ include "app.name" . | quote }}
app.kubernetes.io/instance: {{ .Release.Name | quote }}
{{- end }}

{{/*
Name of deployment.
*/}}
{{- define "app.deploymentName" -}}
{{ include "app.fullname" . }}
{{- end }}

{{/*
Name of service.
*/}}
{{- define "app.serviceName" -}}
{{ include "app.fullname" . }}
{{- end }}

{{/*
Name of container port.
*/}}
{{- define "app.containerPortName" -}}
tcp-echo
{{- end }}

{{/*
Name of service port.
*/}}
{{- define "app.servicePortName" -}}
tcp-echo
{{- end }}

{{/*
Container image tag.
*/}}
{{- define "app.containerImageTag" -}}
{{ .Values.container.image.tag | default .Chart.AppVersion }}
{{- end }}

{{/*
Container image full name.
*/}}
{{- define "app.containerImageFullName" -}}
{{ printf "%s:%s" .Values.container.image.repository (include "app.containerImageTag" . ) }}
{{- end }}

{{/*
Termination grace period.
*/}}
{{- define "app.terminationGracePeriod" -}}
{{ add (.Values.tcpEcho.stopTimeout | default 60) 5 }}
{{- end }}

{{/*
Name of test pod.
*/}}
{{- define "app.testPodName" -}}
{{ include "app.fullname" . }}-test
{{- end }}

{{/*
Test container image full name.
*/}}
{{- define "test.containerImageFullName" -}}
{{ printf "%s:%s" .Values.test.image.repository (.Values.test.image.tag | default "latest") }}
{{- end }}
