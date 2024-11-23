#!/bin/bash

TEST_CASE="case 2 - mkdir"

function check_mkdir () {
    _PARAM=$1
    _TEST_CASE=$2
    if ! stat "$_PARAM" > /dev/null; then
        fail "$_TEST_CASE: stat文件$_PARAM返回值非0"
        return 1
    fi 
    return 0
}

try_mount_or_fail

TEST_CASE="case 2.1 - mkdir ${MNTPOINT}/dir0"
core_tester mkdir "${MNTPOINT}"/dir0 check_mkdir "$TEST_CASE"

TEST_CASE="case 2.2 - mkdir ${MNTPOINT}/dir0/dir0"
core_tester mkdir "${MNTPOINT}"/dir0/dir0 check_mkdir "$TEST_CASE"

TEST_CASE="case 2.3 - mkdir ${MNTPOINT}/dir0/dir0/dir0"
core_tester mkdir "${MNTPOINT}"/dir0/dir0/dir0 check_mkdir "$TEST_CASE"

TEST_CASE="case 2.4 - mkdir ${MNTPOINT}/dir1"
core_tester mkdir "${MNTPOINT}"/dir1 check_mkdir "$TEST_CASE"

