@ECHO OFF

SET ZIPFILE=CLTools.zip 
IF NOT "%1"=="" SET PROGS=%*
IF "%PROGS%"=="" SET PROGS=CLSeek isempty which ifTest CLsync

FOR %%X IN ( %PROGS% ) DO IF NOT EXIST Release\%%X.exe nmake -nologo -f %%X.nmak CFG=Release

REM
FOR %%X IN ( %PROGS% ) DO zip -uv %ZIPFILE% Release\%%X.exe
unzip -qt %ZIPFILE%

SET ZIPFILE=
GOTO :END

:END
GOTO :EOF
