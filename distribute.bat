@ECHO OFF

SET ZIPFILE=CLSeek.zip 
IF "%1"=="" GOTO :EOF
IF NOT "%1"=="" SET PROGS=%*
IF "%PROGS%"=="all" SET PROGS=CLSeek isempty which ifTest CLsync

IF EXIST Release\libtd.lib  nmake -nologo -f libtd.nmak  CFG=Release clean
IF EXIST Release\CLSeek.exe nmake -nologo -f CLSeek.nmak CFG=Release clean
FOR %%X IN ( %PROGS% ) DO nmake -nologo -f %%X.nmak CFG=Release

REM
FOR %%X IN ( %PROGS% ) DO CALL :COPY2NET %%X

REM
zip -uv %ZIPFILE% *.c *.h *nmak* *.bat *.ico
unzip -qt %ZIPFILE%

REM
IF EXIST H:\DWONGNT\Self\Unix\C_progs mv -fvu %ZIPFILE% H:\DWONGNT\Self\Unix\C_progs

FOR %%X IN ( %PROGS% ) DO IF EXIST Debug\%%X.exe nmake -f %%X.nmak clean

SET ZIPFILE=
GOTO :END

REM  subroutine
REM
:COPY2NET
cp -pvu Release\%1.exe C:\PUB
IF EXIST \\cupdavidw02\c$\PUB\ cp -fvu Release\%1.exe \\cupdavidw02\c$\PUB
IF EXIST \\cupwongt01\c$\PUB\  cp -fvu Release\%1.exe \\cupwongt01\c$\PUB
GOTO :EOF


:END
GOTO :EOF
