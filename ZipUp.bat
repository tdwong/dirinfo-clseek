@ECHO OFF

SET ZIPFILE=CLSeek.zip 

zip -uv %ZIPFILE% *.c *.h *nmak*
unzip -t %ZIPFILE%

SET ZIPFILE=
