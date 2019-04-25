#!/bin/bash

LLVM_PROFILE_FILE="${1}".profraw ./"${1}" -nv
