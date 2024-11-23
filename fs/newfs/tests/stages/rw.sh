#!/bin/bash

TEST_CASE="case 6 - read/write"

GOLDEN="Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."

function check_write () {
    _PARAM=$1
    _TEST_CASE=$2
    if ! echo "$_PARAM" | tee "${MNTPOINT}"/file0 > /dev/null; then
        fail "$_TEST_CASE: 写入$_PARAM到文件${MNTPOINT}/file0失败"
        return 1
    fi 
    return 0
}

function check_read () {
    _PARAM=$1
    _TEST_CASE=$2

    if ! cat "${MNTPOINT}"/file0 > /dev/null; then
        fail "$_TEST_CASE: 读文件${MNTPOINT}/file0失败"
        return 1
    fi
    
    OUTPUT=$(cat "${MNTPOINT}"/file0)
    if [[ "${OUTPUT}" != "${GOLDEN}" ]]; then
        fail "$_TEST_CASE: 读文件${MNTPOINT}/file0成功, 但内容不同, 正确的内容为: $GOLDEN"
        return 1
    fi
    return 0
}


try_mount_or_fail

TEST_CASE="case 6.1 - write ${MNTPOINT}/file0"
touch_and_check "${MNTPOINT}"/file0
core_tester echo "$GOLDEN" check_write "$TEST_CASE"

TEST_CASE="case 6.2 - read ${MNTPOINT}/file0"
core_tester echo "$GOLDEN" check_read "$TEST_CASE"