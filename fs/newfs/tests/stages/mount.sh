#!/bin/bash

TEST_CASE="case 1 - mount"

# eqaul to [ $? -ne 0 ]
if ! mount_fuse; then
    fail "$TEST_CASE: mount的返回值非0"
    exit 1
fi

if ! check_mount; then
    fail "$TEST_CASE: mount的返回值为0, 但是没有挂载成功, 请仔细检查"
    exit 1
fi

pass "$TEST_CASE"
