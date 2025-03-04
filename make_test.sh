#!/bin/sh
./make_clean.sh
./make_build.sh
python3 -m pytest test --debug_test     # add -vv for verbose output