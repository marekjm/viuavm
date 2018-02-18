#!/usr/bin/env bash

set -e

./view.py "$@" 2> /dev/null | less -R
