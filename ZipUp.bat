@ECHO OFF

SET ZIPFILE=CLSeek.zip 

zip -uv %ZIPFILE% *.c *.h *nmak* Makefile
unzip -t %ZIPFILE%

SET ZIPFILE=
