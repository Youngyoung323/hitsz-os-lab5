#!/bin/bash

TEST_CASE="case 4 - ls"

# /: dir0 file0
# /dir0: dir1 file1
# /dir0/dir1: dir2 file3
# /dir0/dir1/dir2: file4 file5 file6

RES=( 'dir0 file0' 'dir1 file1' 'dir2 file3' 'file4 file5 file6' )

function check_ls () {
    _PARAM=$1
    _TEST_CASE=$2
    CASE=$(echo "$_TEST_CASE" | awk '{print $2}' | sed "s/4.//g") 
    CASE=$((CASE - 1))
    _RES=(${RES[$CASE]})
    OUTPUT=($(ls "$_PARAM"))
    
    for res in "${_RES[@]}"; do
        IS_FIND=0
        for output in "${OUTPUT[@]}"; do
            if [[ "${res}" == "${output}" ]]; then
                IS_FIND=1
                break
            fi
        done

        if (( IS_FIND != 1 )); then
            fail "$_TEST_CASE: $res没有在ls的输出结果中找到"
            return 1
        fi 
    done
    return 0
}

function create_and_except () {
    mkdir_and_check "${MNTPOINT}"/dir0
    touch_and_check "${MNTPOINT}"/file0
    mkdir_and_check "${MNTPOINT}"/dir0/dir1
    mkdir_and_check "${MNTPOINT}"/dir0/dir1/dir2
    touch_and_check "${MNTPOINT}"/dir0/dir1/dir2/file4
    touch_and_check "${MNTPOINT}"/dir0/dir1/dir2/file5
    touch_and_check "${MNTPOINT}"/dir0/dir1/dir2/file6
    touch_and_check "${MNTPOINT}"/dir0/dir1/file3
    touch_and_check "${MNTPOINT}"/dir0/file1
}

# exit
try_mount_or_fail

create_and_except
# exit
TEST_CASE="case 4.1 - ls ${MNTPOINT}/"
core_tester ls "${MNTPOINT}"/ check_ls "$TEST_CASE"

TEST_CASE="case 4.2 - ls ${MNTPOINT}/dir0"
core_tester ls "${MNTPOINT}"/dir0 check_ls "$TEST_CASE"

TEST_CASE="case 4.3 - ls ${MNTPOINT}/dir0/dir1"
core_tester ls "${MNTPOINT}"/dir0/dir1 check_ls "$TEST_CASE"

TEST_CASE="case 4.4 - ls ${MNTPOINT}/dir0/dir1/dir2"
core_tester ls "${MNTPOINT}"/dir0/dir1/dir2 check_ls "$TEST_CASE"

