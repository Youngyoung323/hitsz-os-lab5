#!/bin/bash

TEST_CASE="case 7 - copy"

GOLDEN="Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."

function check_write () {
    _PARAM=$1
    _TEST_CASE=$2
    if ! echo "$_PARAM" | tee "${MNTPOINT}"/file9 > /dev/null; then
        fail "$_TEST_CASE: 写入$_PARAM到文件${MNTPOINT}/file9失败"
        return 1
    fi 
    return 0
}

function check_read () {
    _PARAM=$1
    _TEST_CASE=$2

    if ! cat "${MNTPOINT}"/file9 > /dev/null; then
        fail "$_TEST_CASE: 读文件${MNTPOINT}/file9失败"
        return 1
    fi
    
    OUTPUT=$(cat "${MNTPOINT}"/file9)
    if [[ "${OUTPUT}" != "${GOLDEN}" ]]; then
        fail "$_TEST_CASE: 读文件${MNTPOINT}/file9成功, 但内容不同, 正确的内容为: $GOLDEN"
        return 1
    fi

    return 0
}

function check_prepare () {
    touch_and_check "${MNTPOINT}"/file9

    if ! check_write "$GOLDEN" "$TEST_CASE"; then
        return 1
    fi

    if ! check_read "$GOLDEN" "$TEST_CASE"; then
        return 1
    fi

    return 0
}

function check_copy () {
    if ! cp "${MNTPOINT}"/file9 "${MNTPOINT}"/file10; then
        fail "$_TEST_CASE: 拷贝文件${MNTPOINT}/file9失败, 返回值非0"
        return 1
    fi

    OUTPUT1=$(cat "${MNTPOINT}"/file9)
    OUTPUT2=$(cat "${MNTPOINT}"/file10)

    if [[ "${OUTPUT1}" != "${OUTPUT2}" ]]; then
        fail "$_TEST_CASE: 拷贝文件${MNTPOINT}/file9至file10成功, 但file10内容不正确, 应该为: $OUTPUT1"
        return 1
    fi
    
    return 0
}


try_mount_or_fail

TEST_CASE="case 7.1 - prepare content of ${MNTPOINT}/file9"
core_tester echo "$TEST_CASE" check_prepare "$TEST_CASE"

TEST_CASE="case 7.2 - copy ${MNTPOINT}/file9 to ${MNTPOINT}/file10"
core_tester echo "$TEST_CASE" check_copy "$TEST_CASE"
