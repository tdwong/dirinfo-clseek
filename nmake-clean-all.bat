@ECHO OFF

rem -- clean Debug & Release using ALL nmake
rem for %%x in ( *.nmak ) DO echo @@@ nmake -f %%x CFG=Debug clean @@@ && nmake -f %%x CFG=Debug clean
FOR %%x in ( *.nmak ) DO (
	FOR %%y in (Debug Release) DO (
		echo @@@ nmake -f %%x CFG=%%y clean @@@ && nmake -f %%x CFG=%%y clean
		)
	)

rem -- delete empty directories
FOR %%y in (Debug Release) DO (
	echo @@@ rmdir %%y @@@ && rmdir %%y
	)

