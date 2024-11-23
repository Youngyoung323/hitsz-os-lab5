#!/bin/bash
POINTS=0
TOTAL_POINTS=0
TEST_CASES=(mount.sh mkdir.sh touch.sh ls.sh remount.sh)
# mount.sh mkdir.sh touch.sh ls.sh remount.sh (read.sh write.sh cp.sh)
ALL_TEST_CASES=(mount.sh mkdir.sh touch.sh ls.sh remount.sh rw.sh cp.sh)
ALL_TEST_SCORES=(1 4 5 4 16 2 2)
MNTPOINT='./mnt'
PROJECT_NAME="newfs"

LEVEL=$1


if [[ "${LEVEL}" == "1" ]]; then
    echo "开始mount测试"
    TEST_CASES=(mount.sh)
    sleep 1
elif [[ "${LEVEL}" == "2" ]]; then
    echo "开始mount, mkdir, touch测试"
    TEST_CASES=(mount.sh mkdir.sh touch.sh)
    sleep 1
elif [[ "${LEVEL}" == "3" ]]; then
    echo "开始mount, mkdir, touch, ls测试"
    TEST_CASES=(mount.sh mkdir.sh touch.sh ls.sh)
    sleep 1
elif [[ "${LEVEL}" == "4" ]]; then
    echo "开始mount, mkdir, touch, ls, umount测试"
    TEST_CASES=(mount.sh mkdir.sh touch.sh ls.sh remount.sh)
    sleep 1
elif [[ "${LEVEL}" == "5" ]]; then
    echo "开始mount, mkdir, touch, ls, read&write, umount测试"
    TEST_CASES=(mount.sh mkdir.sh touch.sh ls.sh remount.sh rw.sh)
    sleep 1
elif [[ "${LEVEL}" == "6" ]]; then
    echo "开始mount, mkdir, touch, ls, read&write, cp, umount测试"
    TEST_CASES=(mount.sh mkdir.sh touch.sh ls.sh remount.sh rw.sh cp.sh)
    sleep 1
else
    echo "未知测试参数"
    exit 1
fi

# Helpers
function get_bash_width() {
    tput cols
}

function repeat_char() {
    start=1
    end=${1:-80}
    str="${2:-=}"
    range=$(seq "$start" "$end")

    for i in $range; do
        _=$i
        echo -n "${str}"
    done
}

function where_is_script() {
    local script=$1
    cd "$(dirname "$script")" && pwd
}

function core_tester() {
    CMD=$1
    PARAM=$2
    CHECK_FN=$3
    TEST_CASE=$4
    SCORE=$5

    "$CMD" "$PARAM" >/dev/null
    if $CHECK_FN "$PARAM" "$TEST_CASE"; then
        pass "$TEST_CASE"
        if [[ -n "${SCORE}" ]]; then
            POINTS=$((POINTS + SCORE - 1))
        fi
    fi
}

function clean_ddriver() {
    sleep 1
    ddriver -r > /dev/null
}

function pass() {
    RES=$1
    POINTS=$((POINTS + 1))
    echo -e "\033[32mpass: ${RES}\033[0m"
}

function fail() {
    RES=$1
    echo -e "\033[31mfail: ${RES}\033[0m"
}

# Utils
function mount_fuse() {
    "$ROOT_PATH"/../build/"${PROJECT_NAME}" --device="$HOME"/ddriver "${MNTPOINT}"
}

function check_mount() {
    ABS_MNTPOINT=$(realpath "$MNTPOINT")
    if ! mount | grep "${ABS_MNTPOINT}" >/dev/null; then
        return 1
    fi
    return 0
}

function try_mount_or_fail() {
    if ! check_mount; then
        mount_fuse
        if ! check_mount; then
            fail "$TEST_CASE: mount的返回值为0, 但是没有挂载成功, 请仔细检查"
            exit 1
        fi
    fi
}

function clean_mount() {
    while true; do
        if ! check_mount; then
            break
        else
            # sudo umount "${MNTPOINT}"
            umount "${MNTPOINT}"
        fi
    done
}

function mkdir_and_check () {
    DIR=$1
    if [ ! -d "$DIR" ]; then
        mkdir "$DIR"
        if ! stat "$DIR" > /dev/null; then
            fail "$TEST_CASE: 目录$DIR创建失败, 请确保能够通过mkdir测试"
            exit
        fi
    fi
}

function touch_and_check () {
    FILE=$1
    if [ ! -f "$FILE" ]; then
        touch "$FILE"
        if ! stat "$FILE" > /dev/null; then
            fail "$TEST_CASE: 文件$FILE创建失败, 请确保能够通过touch测试"
            exit
        fi
    fi
}

# Test
function register_testcase() {
    for target_test_case in "${TEST_CASES[@]}"; do
        ID=0
        echo "测试用例: $ROOT_PATH/stages/$target_test_case"
        for test_case in "${ALL_TEST_CASES[@]}"; do
            if [[ "${test_case}" == "${target_test_case}" ]]; then
                TOTAL_POINTS=$((TOTAL_POINTS + ALL_TEST_SCORES[ID]))
                break
            fi
            ID=$((ID + 1))
        done

    done
}

ROOT_PATH=$(where_is_script "$0")
function prepare_esstential_vars() {
    MNTPOINT="$ROOT_PATH"/mnt
    PROJECT_NAME="newfs"
}

# input: init_tester "${test_cases[@]}"
function init_tester() {
    register_testcase
    prepare_esstential_vars
}

function test_start() {
    TERMINAL_WIDTH=$(get_bash_width)
    clean_mount
    cd "$ROOT_PATH/stages" || exit
    for target_test_case in "${TEST_CASES[@]}"; do
        clean_ddriver
        repeat_char "$TERMINAL_WIDTH" "="
        # shellcheck source=/dev/null
        source ./"$target_test_case"
    done
    repeat_char "$TERMINAL_WIDTH" "="
    cd - >/dev/null || exit
}

function test_end() {
    if [ $POINTS -eq $TOTAL_POINTS ]; then
        echo " "
        echo "Score: $POINTS/$TOTAL_POINTS"
        pass "恭喜你，通过所有测试 ($TOTAL_POINTS/$TOTAL_POINTS)"
    else
        echo " "
        echo "Score: $POINTS/$TOTAL_POINTS"
        fail "再接再厉! ($POINTS/$TOTAL_POINTS)"
    fi
    sleep 1
    clean_mount
}

fg_pid=$$

function handle_timeout() {
    fail "测试超时"
    fs_pid=$(pgrep -u $USER $PROJECT_NAME)
    if [ ! -z "$fs_pid" ]; then
        for PID in $fs_pid; do
            # echo "kill $PID"
            kill -9 $PID
        done
    fi
    fusermount -u $MNTPOINT
    kill -9 $fg_pid
    exit 1
}

# Main
echo "测试脚本工程根目录: $ROOT_PATH"

max_execution_time=100
(
    sleep $max_execution_time
    handle_timeout
) &
timer_pid=$!
# echo "timer $timer_pid"

function main() {
    init_tester
    test_start
    test_end
}

main

kill $timer_pid 2>/dev/null
