@ECHO OFF

ECHO Invoking Visual Studio 2019 Community Edition VCVARS32.BAT
rem
SET VCDIR=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build
IF NOT EXIST "%VCDIR%\VCVARS32.BAT" GOTO :EOF
PUSHD "%VCDIR%"
CALL VCVARS32.BAT
POPD

