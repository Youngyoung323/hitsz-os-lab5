#!/bin/bash

TEST_CASE="case 5 - remount"

RES2=( 'file0 file1 file2' )

function check_umount () {
    _PARAM=$1
    _TEST_CASE=$2
    
    sleep 1
    # sudo umount "${MNTPOINT}"
    umount "${MNTPOINT}"
    
    if ! check_mount; then
        return 0
    fi

    fail "$_TEST_CASE: $PROJECT_NAME文件系统仍然在挂载点${MNTPOINT}"
    return 1
}

function create_and_except_remount () {
    mkdir_and_check "${MNTPOINT}"/dir0
    mkdir_and_check "${MNTPOINT}"/dir0/dir1
    mkdir_and_check "${MNTPOINT}"/dir0/dir1/dir2
    touch_and_check "${MNTPOINT}"/dir0/dir1/dir2/file0
    touch_and_check "${MNTPOINT}"/dir0/dir1/dir2/file1
    touch_and_check "${MNTPOINT}"/dir0/dir1/dir2/file2
}

function create_and_except_bitmap () {
    touch_and_check "${MNTPOINT}/hello"
}

function check_ls_remount () {
    _PARAM=$1
    _TEST_CASE=$2
    _RES=(${RES2[0]})
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
            fail "$_TEST_CASE: $res没有在remount后的ls的输出结果中找到"
            return 1
        fi 
    done
    return 0
}

ERR_OK=0
INODE_MAP_ERR=1
DATA_MAP_ERR=2
LAYOUT_FILE_ERR=3
GOLDEN_LAYOUT_MISMATCH=4
DATA_ERR=5

function check_bm() {
    _PARAM=$1
    _TEST_CASE=$2
    ROOT_PARENT_PATH=$(cd $(dirname $ROOT_PATH); pwd)
    python3 "$ROOT_PATH"/checkbm/checkbm.py -l "$ROOT_PARENT_PATH"/include/fs.layout -r "$ROOT_PARENT_PATH"/tests/checkbm/golden.json > /dev/null
    RET=$?
    if (( RET == ERR_OK )); then
        return 0
    elif (( RET == INODE_MAP_ERR )); then
        fail "$_TEST_CASE: Inode位图错误, 请使用checkbm.py和ddriver工具自行检查. 注: 在命令行输入ddriver -d并且安装HexEditor插件即可查看当前ddriver介质情况"
    elif (( RET == DATA_MAP_ERR )); then
        fail "$_TEST_CASE: 数据位图错误, 请使用checkbm.py和ddriver工具自行检查. 注: 在命令行输入ddriver -d并且安装HexEditor插件即可查看当前ddriver介质情况"
        elif (( RET == DATA_ERR )); then
        fail "$_TEST_CASE: 数据写回错误, 请检查数据是否正确写回到数据区的指定位置"
    elif (( RET == LAYOUT_FILE_ERR )); then
        fail "$_TEST_CASE: .layout文件有误, 请结合报错信息自行检查"
    elif (( RET == GOLDEN_LAYOUT_MISMATCH )); then
        fail "$_TEST_CASE: .layout文件和本次实验布局不符, 请结合报错信息自行检查"
    fi
    return 1
}

clean_mount
clean_ddriver

try_mount_or_fail

create_and_except_remount

TEST_CASE="case 5.1 - umount ${MNTPOINT}"
core_tester ls "${MNTPOINT}" check_umount "$TEST_CASE" 1

sleep 1

try_mount_or_fail


TEST_CASE="case 5.2 - remount ${MNTPOINT}"
core_tester ls "${MNTPOINT}"/dir0/dir1/dir2 check_ls_remount "$TEST_CASE" 3

clean_mount
clean_ddriver

sleep 1

try_mount_or_fail

create_and_except_bitmap

clean_mount

sleep 1


TEST_CASE="case 5.3 - check bitmap"
core_tester ls "${MNTPOINT}" check_bm "$TEST_CASE" 12

clean_mount
clean_ddriver