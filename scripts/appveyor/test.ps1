# Stop immediately if any error happens
$ErrorActionPreference = "Stop"

Set-Location -Path "${env:BUILD_HOME}"
$test_cmd = "ctest.exe --build-config ""${env:CONFIGURATION}"" --verbose"
if (${env:COVERAGE_BUILD} -eq "True") {
  $coverage_report_folder = "${env:COVERAGE_WORK_FOLDER}\report"
  $coverage_report_archive = "${env:COVERAGE_WORK_FOLDER}\report.zip"
  New-Item "${env:COVERAGE_WORK_FOLDER}" -type directory | out-null
  $test_cmd = "OpenCppCoverage.exe --quiet --export_type ""html:${coverage_report_folder}"" --export_type ""cobertura:${env:COBERTURA_COVERAGE_FILE}"" --sources ""${env:APPVEYOR_BUILD_FOLDER}"" --excluded_sources ""${env:APPVEYOR_BUILD_FOLDER}\3rdparty"" --excluded_sources ""${env:APPVEYOR_BUILD_FOLDER}\examples"" --excluded_sources ""${env:APPVEYOR_BUILD_FOLDER}\tests"" --modules ""${env:BUILD_HOME}\tests"" --cover_children --working_dir ""${env:BUILD_HOME}"" -- ${test_cmd}"
}

Invoke-Expression "${test_cmd}"
if (${LastExitCode} -ne 0) {
  throw "Tests failed with exit code ${LastExitCode}"
}

Set-Location -Path "${env:APPVEYOR_BUILD_FOLDER}"

if (${env:COVERAGE_BUILD} -eq "True") {
  7z.exe a -tzip "${coverage_report_archive}" "${coverage_report_folder}" | out-null
  if (${LastExitCode} -ne 0) {
    throw "Failed to archive coverage report located at ${coverage_report_folder} with exit code ${LastExitCode}"
  }
  Push-AppveyorArtifact "${coverage_report_archive}" -DeploymentName "${env:COVERAGE_ARTIFACT_NAME}"
  Push-AppveyorArtifact "${env:COBERTURA_COVERAGE_FILE}" -DeploymentName "${env:COVERAGE_ARTIFACT_NAME}"

  codecov --token "${env:CODECOV_TOKEN}" --file "${env:COBERTURA_COVERAGE_FILE}" --root "${env:APPVEYOR_BUILD_FOLDER}" -X gcov -F "${env:CODECOV_FLAG}"
}
