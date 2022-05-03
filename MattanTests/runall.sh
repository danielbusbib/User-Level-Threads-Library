#!/bin/bash

TEST_DIR="${0%/*}"

BUILD_DIR="${TEST_DIR}/../cmake-build-debug"
TEST_EXEC="${BUILD_DIR}/tests/theTests"

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}
cmake ..
make

TESTS=`${TEST_EXEC} --gtest_list_tests | grep '^Test'`
FAIL=0
SUCCESS=0
for TEST in $TESTS
do
  (${TEST_EXEC} --gtest_filter="${TEST}*")
  if [ $? -eq 0 ]
  then
    SUCCESS=$((SUCCESS + 1))
  else
    FAIL=$((FAIL + 1))
    fi
done

if [ $FAIL != 0 ]
then
  echo "${FAIL} tests have failed"
echo "${SUCCESS} tests have succeeded"
else
  echo "All ${SUCCESS} tests have succeeded"
fi

