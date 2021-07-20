#!/usr/bin/env bash

set -e

cat $(find ./include ./src -type f) | sha384sum | cut -d' ' -f1
