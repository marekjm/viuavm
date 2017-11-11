#!/usr/bin/env bash

notify-send --urgency normal --expire-time 4000 'Viua VM recompilation engine' "Compilation triggered"

OUTPUT_FILE=/tmp/viuavm_compile
STATUS_FILE=/tmp/viuavm_compile_exit_status

(/usr/bin/time -f 'Wall clock: %E CPU: %P' ./scripts/compile 2>&1 ; echo "$?" > $STATUS_FILE) | tee $OUTPUT_FILE
ELAPSED=$(tail -n 1 $OUTPUT_FILE)

if [[ $(cat $STATUS_FILE) -eq 0 ]]; then
    notify-send --urgency normal --expire-time 4000 'Viua VM recompilation engine' "Recompiled code\n$ELAPSED"
else
    notify-send --urgency critical --expire-time 5000 'Viua VM recompilation engine' "Compilation failed\n$ELAPSED"
fi
