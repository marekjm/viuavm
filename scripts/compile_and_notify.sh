#!/usr/bin/env bash

#
#   Copyright (C) 2018 Marek Marecki
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

# Send a notification to tell the developer that the compilation has started.
# Lack of this notification means that something broke and the script may need to be
# restarted manually.
notify-send --urgency normal --expire-time 4000 'Viua VM recompilation engine' "Compilation triggered"


OUTPUT_FILE=/tmp/viuavm_compile
STATUS_FILE=/tmp/viuavm_compile_exit_status
OK_NOTIF_FILE=/tmp/viuavm_compile_success_notification
ERR_NOTIF_FILE=/tmp/viuavm_compile_error_notification

# Pipe all the output to file (to allow parsing) *AND* to stdout so that you can see what
# happened without having to open the log file.
(/usr/bin/time -f 'Wall clock: %E CPU: %P' ./scripts/compile $1 2>&1 ; echo "$?" > $STATUS_FILE) | tee $OUTPUT_FILE
ELAPSED=$(tail -n 1 $OUTPUT_FILE)

# Notify the developer about success or failure of the build.
if [[ $(cat $STATUS_FILE) -eq 0 ]]; then
    notify-send --urgency normal --expire-time 4000 'Viua VM recompilation engine' "Recompiled code\n$ELAPSED"
    dd if=/dev/urandom count=1 2>/dev/null | sha1sum > $OK_NOTIF_FILE
else
    notify-send --urgency critical --expire-time 5000 'Viua VM recompilation engine' "Compilation failed\n$ELAPSED"
    dd if=/dev/urandom count=1 2>/dev/null | sha1sum > $ERR_NOTIF_FILE
fi
