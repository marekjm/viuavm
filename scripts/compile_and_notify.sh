#!/usr/bin/env bash

# Send a notification to tell the developer that the compilation has started.
# Lack of this notification means that something broke and the script may need to be
# restarted manually.
notify-send --urgency normal --expire-time 4000 'Viua VM recompilation engine' "Compilation triggered"


OUTPUT_FILE=/tmp/viuavm_compile
STATUS_FILE=/tmp/viuavm_compile_exit_status

# Pipe all the output to file (to allow parsing) *AND* to stdout so that you can see what
# happened without having to open the log file.
(/usr/bin/time -f 'Wall clock: %E CPU: %P' ./scripts/compile 2>&1 ; echo "$?" > $STATUS_FILE) | tee $OUTPUT_FILE
ELAPSED=$(tail -n 1 $OUTPUT_FILE)

# Notify the developer about success or failure of the build.
if [[ $(cat $STATUS_FILE) -eq 0 ]]; then
    notify-send --urgency normal --expire-time 4000 'Viua VM recompilation engine' "Recompiled code\n$ELAPSED"
else
    notify-send --urgency critical --expire-time 5000 'Viua VM recompilation engine' "Compilation failed\n$ELAPSED"
fi
