#!/usr/bin/env sh

#
#   Copyright (C) 2015, 2016 Marek Marecki
#
#   This file is part of Viua VM.
#
#   Viua VM is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Viua VM is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
#

FILE=include/viua/version.h
COMMITS_SINCE=`git describe HEAD | cut -d'-' -f2`
SAVED_MICRO=`grep -P 'MICRO' $FILE | grep -Po '\d+'`

if [[ $COMMITS_SINCE == $SAVED_MICRO ]]; then
    exit 0
fi

HEAD_UPDATED_VERSION=`git show HEAD | grep -P '^---' | cut -d' ' -f2 | sed 's:a/::' | grep $FILE`

if [[ $HEAD_UPDATED_VERSION == "" ]]; then
    sed -i "s/COMMIT = \".*\"/COMMIT = \"`git rev-parse HEAD`\"/" $FILE
    sed -i "s/MICRO = \".*\"/MICRO = \"$COMMITS_SINCE\"/" $FILE
fi
