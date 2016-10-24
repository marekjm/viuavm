#!/usr/bin/env bash

set -e

VIUA_BENCH_ITERATIONS=4 VIUA_BENCH_SCHED_COUNTS='1 2 4 8' VIUA_BENCH_KERNEL=./build/bin/vm/kernel ./benchmark_bottles_viua.sh '1024 4096 8192 16384 65536' 2>&1
