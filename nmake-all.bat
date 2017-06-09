@ECHO OFF

rem -- LIST= CLSeek.nmak CLsync.nmak delempty.nmak dirinfo.nmak ifTest.nmak iregex.nmak isempty.nmak libtd.nmak regex.nmak tryrun.nmak which.nmak

rem -- build using ALL nmake
FOR %%x in ( *.nmak ) DO echo @@@ nmake -f %%x @@@ && nmake -f %%x

echo.
echo.

rem -- print version...
pushd Debug
FOR %%x in ( *.exe ) DO echo @@@ %%x @@@ && %%x -v
popd

