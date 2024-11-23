#!/bin/bash
#!/bin/bash

TEST_CASE="case 3 - touch"

function check_touch () {
    _PARAM=$1
    _TEST_CASE=$2
    if ! stat "$_PARAM" > /dev/null; then
        fail "$_TEST_CASE: stat文件$_PARAM返回值非0"
        return 1
    fi 
    return 0
}

try_mount_or_fail

TEST_CASE="case 3.1 - touch ${MNTPOINT}/file0"
core_tester touch "${MNTPOINT}"/file0 check_touch "$TEST_CASE"

TEST_CASE="case 3.2 - touch ${MNTPOINT}/file1"
core_tester touch "${MNTPOINT}"/file1 check_touch "$TEST_CASE"

TEST_CASE="case 3.3 - touch ${MNTPOINT}/dir0/file1"
mkdir_and_check "${MNTPOINT}"/dir0
core_tester touch "${MNTPOINT}"/dir0/file1 check_touch "$TEST_CASE"

TEST_CASE="case 3.4 - touch ${MNTPOINT}/dir0/file2"
core_tester touch "${MNTPOINT}"/dir0/file2 check_touch "$TEST_CASE"

TEST_CASE="case 3.5 - touch ${MNTPOINT}/dir1/file3"
mkdir_and_check "${MNTPOINT}"/dir1
core_tester touch "${MNTPOINT}"/dir1/file3 check_touch "$TEST_CASE"

