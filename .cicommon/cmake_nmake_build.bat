MD build
PUSHD build
SET CMAKE_BUILD_TYPE=%~1
IF "%CMAKE_BUILD_TYPE%"=="" SET CMAKE_BUILD_TYPE=Release
cmake -DCMAKE_BUILD_TYPE="%CMAKE_BUILD_TYPE%" -G "NMake Makefiles" ..
cmake --build . --target install --config Release --clean-first
POPD
