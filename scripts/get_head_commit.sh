#!/usr/bin/env bash

set -e

# If in a repository then just use HEAD hash as build commit hash.
if [[ -d .git ]]; then
    git rev-parse HEAD
    exit 0
fi

# If not in a repository then check if there is a VIUA_VM_COMMIT file which
# should contain the hash of a commit from which this archive (you got this
# code from a tar archive, didn't you?) was made.
if [[ -f VIUA_VM_COMMIT ]]; then
    cat VIUA_VM_COMMIT
    exit 0
fi

# Whoa, nothing matched? Tough luck, we have to just throw our hands up and
# refuse to work. Let's echo some borked commit hash and notify about the
# error via exit code - people who just want to get something hash-looking
# will get a nice string of zeroes (always a good default), and people who
# want to detect errors will get a nice exit code.
echo '0000000000000000000000000000000000000000'
exit 1
