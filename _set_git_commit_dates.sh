#!/bin/bash
#

[ $# -eq 0 ] && echo "upk <version>" && exit

f=~/OneDrive/Workspace/CLSeek-$1.zip
[ ! -f $f ] && echo "$f: missing zip archive" && exit

unzip -xo $f

TARGET=CLSeek.c

export GIT_AUTHOR_NAME="Tzunghsing D. Wong"
export GIT_AUTHOR_EMAIL="tdwong@yahoo.com"
export GIT_COMMITTER_NAME=$GIT_AUTHOR_NAME
export GIT_COMMITTER_EMAIL=$GIT_AUTHOR_EMAIL

if [ "$(uname -s)" = "Linux" ]; then
	export GIT_AUTHOR_DATE=$( stat -c %Y $TARGET )
else	# [ "$(uname -s)" = "Darwin" ]
	export GIT_AUTHOR_DATE=$( stat -f %m $TARGET )
fi
export GIT_COMMITTER_DATE=$GIT_AUTHOR_DATE

echo "GIT_AUTHOR_NAME=$GIT_AUTHOR_NAME"
echo "GIT_AUTHOR_EMAIL=$GIT_AUTHOR_EMAIL"
echo "GIT_AUTHOR_DATE=$GIT_AUTHOR_DATE"

