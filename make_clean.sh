#!/bin/sh
rm -rf build/
rm -rf test/__pycache__/
rm -rf test_files/
rm -rf debug/
rm -rf .pytest_cache/
rm book_*.txt
rm test_*_text.txt
rm map_results.txt
rm own_test_simple_output.txt
rm src/lib/tests/a.out
rm src/lib/tests/*.o
rm src/lib/tests/valgrind-out.txt
rm valgrind_output_worker_*.xml
rm valgrind_output_distributor.xml
rm valgrind_test.txt
rm interop_test.txt