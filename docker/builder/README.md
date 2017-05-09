# Usage

## Alpine Linux

Building with Qt 5.x and with calculating of code coverage:

```
docker run --rm --env MA_QT_MAJOR_VERSION=5 --env BUILD_TYPE=DEBUG --env COVERAGE_BUILD=ON -v <directory_with_project>:/project -v <directory_with_results_of_build>:/build abrarov/asio-samples-builder-alpine
```

Building of release version with Qt 5.x:

```
docker run --rm --env MA_QT_MAJOR_VERSION=5 -v <directory_with_project>:/project -v <directory_with_results_of_build>:/build abrarov/asio-samples-builder-alpine
```

Building with Qt 4.x and with calculating of code coverage:

```
docker run --rm --env MA_QT_MAJOR_VERSION=4 --env BUILD_TYPE=DEBUG --env COVERAGE_BUILD=ON -v <directory_with_project>:/project -v <directory_with_results_of_build>:/build abrarov/asio-samples-builder-alpine
```

Building of release version with Qt 4.x:

```
docker run --rm --env MA_QT_MAJOR_VERSION=4 -v <directory_with_project>:/project -v <directory_with_results_of_build>:/build abrarov/asio-samples-builder-alpine
```