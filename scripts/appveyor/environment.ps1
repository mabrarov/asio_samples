# Stop immediately if any error happens
$ErrorActionPreference = "Stop"

if (Test-Path env:QT_VERSION) {
  $env:PATH = "${env:QT_BIN_FOLDER};${env:PATH}"
}
if ((Test-Path env:BOOST_VERSION) -and (${env:BOOST_LINKAGE} -ne "static")) {
  $env:PATH = "${env:BOOST_LIBRARY_FOLDER};${env:PATH}"
}
if ((Test-Path env:ICU_VERSION) -and (${env:ICU_LINKAGE} -ne "static")) {
  $env:PATH = "${env:ICU_LIBRARY_FOLDER};${env:PATH}"
}
if (Test-Path env:MINGW_HOME) {
  $env:PATH = "${env:MINGW_HOME}\bin;${env:PATH}"
}
