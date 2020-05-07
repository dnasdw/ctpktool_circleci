PUSHD "%~dp0"
FOR /F %%I IN ('REG QUERY HKLM\SOFTWARE\WOW6432Node\Microsoft ^| FINDSTR "VisualStudio_"') DO CALL :FINDVS "%%~I" 2019
CALL %VCVARSALL% "%~1"
@ECHO ON
POPD
GOTO :EOF

:FINDVS
SET vsid=%~1
SET vsid=%vsid:~63%
REG QUERY %~1\Capabilities | FINDSTR /R "ApplicationName.*REG_SZ.*Microsoft.Visual.Studio.%~2" && FOR /F "tokens=1,2,*" %%I IN ('REG QUERY HKLM\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\%vsid% ^| FINDSTR "InstallLocation"') DO SET VCVARSALL="%%~K\VC\Auxiliary\Build\vcvarsall.bat"
GOTO :EOF
