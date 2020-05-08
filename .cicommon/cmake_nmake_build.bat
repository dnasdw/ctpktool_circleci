MKDIR build
PUSHD build
SET CMAKE_BUILD_TYPE=%~1
IF "%CMAKE_BUILD_TYPE%"=="" SET CMAKE_BUILD_TYPE=Release
cmake -DCMAKE_BUILD_TYPE="%CMAKE_BUILD_TYPE%" -G "NMake Makefiles" .. || GOTO ERROR
cmake --build . --target install --config "%CMAKE_BUILD_TYPE%" --clean-first || GOTO ERROR
POPD
GOTO :EOF

:ERROR
EXIT /B %ERRORLEVEL%
GOTO :EOF
