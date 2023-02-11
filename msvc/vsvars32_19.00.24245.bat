@ECHO OFF

ECHO Invoking REAL vsvars32.bat

rem -- Visual Studio 2015 Community Edition --
SET VCDIR=C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools
IF NOT EXIST "%VCDIR%\VSVARS32.BAT" GOTO :EOF
PUSHD "%VCDIR%"
CALL VSVARS32.BAT
POPD


