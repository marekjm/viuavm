#!/usr/bin/env sh

FILE=src/version.h
COMMITS_SINCE=`git describe HEAD | cut -d'-' -f2`
SAVED_MICRO=`grep -P 'MICRO' $FILE | grep -Po '\d+'`

# add 1 to include the *just being commited* commit
COMMITS_SINCE=`dc -e "1 $COMMITS_SINCE + p"`

if [[ $COMMITS_SINCE == $SAVED_MICRO ]]; then
    exit 0
fi

HEAD_UPDATED_VERSION=`git show HEAD | grep -P '^---' | cut -d' ' -f2 | sed 's:a/::' | grep $FILE`

if [[ $HEAD_UPDATED_VERSION == "" ]]; then
    sed -i "s/COMMIT = \".*\"/COMMIT = \"`git rev-parse HEAD`\"/" $FILE
    sed -i "s/MICRO = \".*\"/MICRO = \"$COMMITS_SINCE\"/" $FILE
fi
