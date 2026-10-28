#!/usr/bin/env bash
# Pruebas del motor DSP en Linux/macOS (g++ o clang++). Uso: ./tests/run_tests.sh [--demos carpeta]
set -e
cd "$(dirname "$0")/.."
${CXX:-g++} -std=c++17 -O2 -Wall -Wextra -o /tmp/dbf_dsp_test tests/dsp_test.cpp
/tmp/dbf_dsp_test "$@"
