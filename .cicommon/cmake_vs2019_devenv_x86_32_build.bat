MKDIR build
PUSHD build
SET CMAKE_BUILD_TYPE=%~1
IF "%CMAKE_BUILD_TYPE%"=="" SET CMAKE_BUILD_TYPE=Release
IF NOT "%CMAKE_BUILD_TYPE%"=="Debug" (
    IF NOT "%CMAKE_BUILD_TYPE%"=="Release" (
        IF NOT "%CMAKE_BUILD_TYPE%"=="RelWithDebInfo" (
            IF NOT "%CMAKE_BUILD_TYPE%"=="MinSizeRel" (
                SET CMAKE_BUILD_TYPE=Release
            )
        )
    )
)
cmake -G "Visual Studio 16" -T v142 -A Win32 ..
cmake --build . --target install --config %CMAKE_BUILD_TYPE% --clean-first
POPD
