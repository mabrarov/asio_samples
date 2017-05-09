# Usage of docker images for building

## Common

&lt;directory_with_project&gt; is assumed to be a directory with project (root). 
&lt;directory_with_results_of_build&gt; is assumed to be existing directory where the results of build (CMake generated project) will be placed.

## Alpine Linux

Use `abrarov/asio-samples-builder-alpine` as &lt;image-name&gt;.

## Ubuntu

Use `abrarov/asio-samples-builder-ubuntu` as &lt;image-name&gt;.

## Build steps

Use below command to build with Qt 5.x and to calculate code coverage:

```
docker run --rm --env MA_QT_MAJOR_VERSION=5 --env BUILD_TYPE=DEBUG --env COVERAGE_BUILD=ON -v <directory_with_project>:/project -v <directory_with_results_of_build>:/build <image-name>
```

Use below command to build release version with Qt 5.x:

```
docker run --rm --env MA_QT_MAJOR_VERSION=5 -v <directory_with_project>:/project -v <directory_with_results_of_build>:/build <image-name>
```

Use below command to build with Qt 4.x and to calculate code coverage:

```
docker run --rm --env MA_QT_MAJOR_VERSION=4 --env BUILD_TYPE=DEBUG --env COVERAGE_BUILD=ON -v <directory_with_project>:/project -v <directory_with_results_of_build>:/build <image-name>
```

Use below command to build release version with Qt 4.x:

```
docker run --rm --env MA_QT_MAJOR_VERSION=4 -v <directory_with_project>:/project -v <directory_with_results_of_build>:/build <image-name>
```
