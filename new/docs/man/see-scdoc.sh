#!/usr/bin/env bash

set -e

#
# -T utf8   sets output encoding
# -K utf    sets input encoding
#
# BOTH options are important!
cat "${1}" | m4 | scdoc | groff -m an -K utf8 -T utf8 -t -
