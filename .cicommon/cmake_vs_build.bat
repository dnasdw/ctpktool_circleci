::  input:
::      %A_VS_VERSION%
::          [vs2008|vs2010|vs2012|vs2013|vs2015|vs2017|vs2019]
::      %A_VS_TOOL%
::          [devenv|nmake]
::      %A_WINDOWS_VERSION%
::          [winxp|win7]
::          only for vs2012/vs2013/vs2015/vs2017 devenv
::      %A_CMAKE_OPTIONS%
::          use "--" for no option
::          for example "-C <initial-cache> -D <var>:<type>=<value>"
::      %A_CMAKE_BUILD_ARCH%
::          [x86_32|x86_64]
::      %A_CMAKE_BUILD_TYPE%
::          [Debug|Release|RelWithDebInfo|MinSizeRel]
::      %A_CMAKE_BUILD_TARGET%
::          use "--" for default target
::          for example "install"

::  var:
::      %CMAKE_OPTIONS%
::      %CMAKE_BUILD_TYPE%
::      !CMAKE_GENERATOR!
::          only for devenv
::      !CMAKE_TOOLSET!
::          only for vs2012/vs2013/vs2015/vs2017/vs2019 devenv
::      !CMAKE_PLATFORM!
::          only for vs2019 devenv
::      %CMAKE_BUILD_TARGET%
::      !VCVARSALL!
::          only for nmake
::      !VC_ARCH!
::          only for nmake

SETLOCAL ENABLEDELAYEDEXPANSION

SET A_VS_VERSION=%~1
SET A_VS_TOOL=%~2
SET A_WINDOWS_VERSION=%~3
SET A_CMAKE_OPTIONS=%~4
SET A_CMAKE_BUILD_ARCH=%~5
SET A_CMAKE_BUILD_TYPE=%~6
SET A_CMAKE_BUILD_TARGET=%~7

IF NOT "%A_VS_VERSION%"=="vs2008" (
    IF NOT "%A_VS_VERSION%"=="vs2010" (
        IF NOT "%A_VS_VERSION%"=="vs2012" (
            IF NOT "%A_VS_VERSION%"=="vs2013" (
                IF NOT "%A_VS_VERSION%"=="vs2015" (
                    IF NOT "%A_VS_VERSION%"=="vs2017" (
                        IF NOT "%A_VS_VERSION%"=="vs2019" (
                            GOTO ERROR
                        )
                    )
                )
            )
        )
    )
)

IF NOT "%A_VS_TOOL%"=="devenv" (
    IF NOT "%A_VS_TOOL%"=="nmake" (
        GOTO ERROR
    )
)

IF NOT "%A_WINDOWS_VERSION%"=="winxp" (
    IF NOT "%A_WINDOWS_VERSION%"=="win7" (
        GOTO ERROR
    )
)

IF NOT "%A_CMAKE_BUILD_ARCH%"=="x86_32" (
    IF NOT "%A_CMAKE_BUILD_ARCH%"=="x86_64" (
        GOTO ERROR
    )
)

IF NOT "%A_CMAKE_BUILD_TYPE%"=="Debug" (
    IF NOT "%A_CMAKE_BUILD_TYPE%"=="Release" (
        IF NOT "%A_CMAKE_BUILD_TYPE%"=="RelWithDebInfo" (
            IF NOT "%A_CMAKE_BUILD_TYPE%"=="MinSizeRel" (
                GOTO ERROR
            )
        )
    )
)

SET CMAKE_OPTIONS=%A_CMAKE_OPTIONS%

SET CMAKE_BUILD_TYPE=%A_CMAKE_BUILD_TYPE%

SET CMAKE_BUILD_TARGET=%A_CMAKE_BUILD_TARGET%

IF "%A_VS_TOOL%"=="devenv" (
    IF "%A_VS_VERSION%"=="vs2008" (
        IF "%A_CMAKE_BUILD_ARCH%"=="x86_32" (
            SET CMAKE_GENERATOR=Visual Studio 9 2008
        ) ELSE IF "%A_CMAKE_BUILD_ARCH%"=="x86_64" (
            SET CMAKE_GENERATOR=Visual Studio 9 2008 Win64
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2010" (
        IF "%A_CMAKE_BUILD_ARCH%"=="x86_32" (
            SET CMAKE_GENERATOR=Visual Studio 10
        ) ELSE IF "%A_CMAKE_BUILD_ARCH%"=="x86_64" (
            SET CMAKE_GENERATOR=Visual Studio 10 Win64
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2012" (
        IF "%A_CMAKE_BUILD_ARCH%"=="x86_32" (
            SET CMAKE_GENERATOR=Visual Studio 11
        ) ELSE IF "%A_CMAKE_BUILD_ARCH%"=="x86_64" (
            SET CMAKE_GENERATOR=Visual Studio 11 Win64
        )
        IF "%A_WINDOWS_VERSION%"=="winxp" (
            SET CMAKE_TOOLSET=v110_xp
        ) ELSE (
            SET CMAKE_TOOLSET=v110
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2013" (
        IF "%A_CMAKE_BUILD_ARCH%"=="x86_32" (
            SET CMAKE_GENERATOR=Visual Studio 12
        ) ELSE IF "%A_CMAKE_BUILD_ARCH%"=="x86_64" (
            SET CMAKE_GENERATOR=Visual Studio 12 Win64
        )
        IF "%A_WINDOWS_VERSION%"=="winxp" (
            SET CMAKE_TOOLSET=v120_xp
        ) ELSE (
            SET CMAKE_TOOLSET=v120
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2015" (
        IF "%A_CMAKE_BUILD_ARCH%"=="x86_32" (
            SET CMAKE_GENERATOR=Visual Studio 14
        ) ELSE IF "%A_CMAKE_BUILD_ARCH%"=="x86_64" (
            SET CMAKE_GENERATOR=Visual Studio 14 Win64
        )
        IF "%A_WINDOWS_VERSION%"=="winxp" (
            SET CMAKE_TOOLSET=v140_xp
        ) ELSE (
            SET CMAKE_TOOLSET=v140
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2017" (
        IF "%A_CMAKE_BUILD_ARCH%"=="x86_32" (
            SET CMAKE_GENERATOR=Visual Studio 15
        ) ELSE IF "%A_CMAKE_BUILD_ARCH%"=="x86_64" (
            SET CMAKE_GENERATOR=Visual Studio 15 Win64
        )
        IF "%A_WINDOWS_VERSION%"=="winxp" (
            SET CMAKE_TOOLSET=v141_xp
        ) ELSE (
            SET CMAKE_TOOLSET=v141
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2019" (
        SET CMAKE_GENERATOR=Visual Studio 16
        SET CMAKE_TOOLSET=v142
        IF "%A_CMAKE_BUILD_ARCH%"=="x86_32" (
            SET CMAKE_PLATFORM=Win32
        ) ELSE IF "%A_CMAKE_BUILD_ARCH%"=="x86_64" (
            SET CMAKE_PLATFORM=x64
        )
    )

    MKDIR build
    PUSHD build
    IF "%A_VS_VERSION%"=="vs2008" (
        cmake %CMAKE_OPTIONS% -G !%CMAKE_GENERATOR%! .. || (
            POPD
            GOTO ERROR
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2010" (
        cmake %CMAKE_OPTIONS% -G !%CMAKE_GENERATOR%! .. || (
            POPD
            GOTO ERROR
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2012" (
        cmake %CMAKE_OPTIONS% -G "!CMAKE_GENERATOR!" -T !CMAKE_TOOLSET! .. || (
            POPD
            GOTO ERROR
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2013" (
        cmake %CMAKE_OPTIONS% -G "!CMAKE_GENERATOR!" -T !CMAKE_TOOLSET! .. || (
            POPD
            GOTO ERROR
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2015" (
        cmake %CMAKE_OPTIONS% -G "!CMAKE_GENERATOR!" -T !CMAKE_TOOLSET! .. || (
            POPD
            GOTO ERROR
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2017" (
        cmake %CMAKE_OPTIONS% -G "!CMAKE_GENERATOR!" -T !CMAKE_TOOLSET! .. || (
            POPD
            GOTO ERROR
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2019" (
        cmake %CMAKE_OPTIONS% -G "!CMAKE_GENERATOR!" -T !CMAKE_TOOLSET! -A !CMAKE_PLATFORM! .. || (
            POPD
            GOTO ERROR
        )
    )
    IF "%CMAKE_BUILD_TARGET%"=="--" (
        cmake --build . --config %CMAKE_BUILD_TYPE% --clean-first || (
            POPD
            GOTO ERROR
        )
    ) ELSE (
        cmake --build . --target "%CMAKE_BUILD_TARGET%" --config %CMAKE_BUILD_TYPE% --clean-first || (
            POPD
            GOTO ERROR
        )
    )
    POPD
) ELSE IF "%A_VS_TOOL%"=="nmake" (
    IF "%A_VS_VERSION%"=="vs2008" (
        IF DEFINED VS90COMNTOOLS (
            SET VCVARSALL="%VS90COMNTOOLS%..\..\VC\vcvarsall.bat"
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2010" (
        IF DEFINED VS100COMNTOOLS (
            SET VCVARSALL="%VS100COMNTOOLS%..\..\VC\vcvarsall.bat"
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2012" (
        IF DEFINED VS110COMNTOOLS (
            SET VCVARSALL="%VS110COMNTOOLS%..\..\VC\vcvarsall.bat"
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2013" (
        IF DEFINED VS120COMNTOOLS (
            SET VCVARSALL="%VS120COMNTOOLS%..\..\VC\vcvarsall.bat"
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2015" (
        IF DEFINED VS140COMNTOOLS (
            SET VCVARSALL="%VS140COMNTOOLS%..\..\VC\vcvarsall.bat"
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2017" (
        FOR /F "tokens=1,2,*" %%I IN ('REG QUERY HKLM\SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7 /v 15.0 ^| FINDSTR "15.0"') DO (
            SET VCVARSALL="%%~KVC\Auxiliary\Build\vcvarsall.bat"
        )
    ) ELSE IF "%A_VS_VERSION%"=="vs2019" (
        FOR /F %%I IN ('REG QUERY HKLM\SOFTWARE\WOW6432Node\Microsoft ^| FINDSTR "VisualStudio_"') DO (
            CALL :FINDVS "%%~I" 2019
        )
    )
    IF NOT DEFINED VCVARSALL (
        GOTO ERROR
    )

    IF "%A_CMAKE_BUILD_ARCH%"=="x86_32" (
        SET VC_ARCH=x86
    ) ELSE IF "%A_CMAKE_BUILD_ARCH%"=="x86_64" (
        SET VC_ARCH=amd64
    )

    CALL !VCVARSALL! !VC_ARCH!

    MKDIR build
    PUSHD build
    cmake %CMAKE_OPTIONS% -D CMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% -G "NMake Makefiles" .. || (
        POPD
        GOTO ERROR
    )
    IF "%CMAKE_BUILD_TARGET%"=="--" (
        cmake --build . --config %CMAKE_BUILD_TYPE% --clean-first || (
            POPD
            GOTO ERROR
        )
    ) ELSE (
        cmake --build . --target "%CMAKE_BUILD_TARGET%" --config %CMAKE_BUILD_TYPE% --clean-first || (
            POPD
            GOTO ERROR
        )
    )
    POPD
)

ENDLOCAL
GOTO :EOF

:FINDVS
SET vsid=%~1
SET vsid=%vsid:~63%
REG QUERY %~1\Capabilities | FINDSTR /R "ApplicationName.*REG_SZ.*Microsoft.Visual.Studio.%~2" && FOR /F "tokens=1,2,*" %%I IN ('REG QUERY HKLM\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\%vsid% ^| FINDSTR "InstallLocation"') DO (
    SET VCVARSALL="%%~K\VC\Auxiliary\Build\vcvarsall.bat"
)
GOTO :EOF

:ERROR
ENDLOCAL
EXIT /B 1
GOTO :EOF
