import argparse
from distutils.log import error
from io import TextIOWrapper
import os
import sys
import json
from typing import List

""" Error Code """
ERR_OK = 0
INODE_MAP_ERR = 1
DATA_MAP_ERR = 2
LAYOUT_FILE_ERR = 3
GOLDEN_LAYOUT_MISMATCH = 4
DATA_ERR = 5

""" Messages """
ERROR = "错误: "

""" Tokens """
SUPER = [ "super" ]
INODE_MAP = [ "inode", "map" ]
DATA_MAP = [ "data", "map" ]
INODE = [ "inode" ]
TOKENS_TOBE_CHECK = [SUPER, INODE_MAP, DATA_MAP, INODE]

""" Path Resolution """
root = os.path.split(os.path.realpath(__file__))[0]
home = os.environ['HOME']
ddriver = home + "/ddriver"

parser = argparse.ArgumentParser()
parser.add_argument("-l", "--layout", help="absolute path of .layout file")
parser.add_argument("-r", "--rules", help="absolute path of golden rule json file")
args = parser.parse_args() 

if args.layout == None:
    fslayout = root + "/../../include/fs.layout"
else:
    fslayout = args.layout

if args.rules == None:
    golden = root + "/golden.json"
else:
    golden = args.rules

""" Does layout contain token """
def check_layout(layout: str, tokens: List[str]):
    layout_lower = layout.lower()
    for token in tokens:
        if layout_lower.find(token) == -1:
            return False
    return True

""" Parse Golden Rules """
valid_inode = 0
valid_data = 0
with open(golden, "r") as f:
    golden_rules:dict = json.load(f)
    layouts = golden_rules.get("checks")
    valid_inode = golden_rules.get("valid_inode")
    valid_data = golden_rules.get("valid_data")
    _TOKENS_TOBE_CHECK = []
    for layout in layouts:
        if check_layout(layout, SUPER):
            _TOKENS_TOBE_CHECK.append(SUPER)
        elif check_layout(layout, INODE_MAP):
            _TOKENS_TOBE_CHECK.append(INODE_MAP)
        elif check_layout(layout, DATA_MAP):
            _TOKENS_TOBE_CHECK.append(DATA_MAP)
        elif check_layout(layout, INODE):
            _TOKENS_TOBE_CHECK.append(INODE)
    if len(_TOKENS_TOBE_CHECK) != 0:
        TOKENS_TOBE_CHECK = _TOKENS_TOBE_CHECK
print("\n" + golden + " parsed results:")
print("\tcheck layouts: [", end=" ")
is_check_data_map = False
is_check_inode_map = False
is_check_data = False
for token in TOKENS_TOBE_CHECK:
    if token == SUPER:
        print("super", end=" ")
    if token == DATA_MAP:
        print("data_map", end=" ")
        is_check_data_map = True    
    if token == INODE_MAP:
        print("inode_map", end=" ")
        is_check_inode_map = True
    if token == INODE:
        print("inode_map", end=" ")
        is_check_data = True
print("]")
if is_check_inode_map:
    print("\tvalid_inode: " + str(valid_inode))
if is_check_data_map:
    print("\tvalid_data: " + str(valid_data))


""" Parse Layout Info """
block_size = 0
superblock_ofs = 0 
superblock_blks = 0 
inode_map_ofs = 0 
inode_map_blks = 0 
data_map_ofs = 0 
data_map_blks = 0  
inode_ofs = 0 
inode_blks = 0 
with open(fslayout, "r") as f:
    lines = f.readlines()
    layoutsln = ""
    """ parse block size """
    for line in lines:
        line = line.strip(" ")
        if line.find("BSIZE") != -1:
            tokens = line.split(" ")
            for token in tokens:
                if token.isdigit():
                    block_size = int(token)
    """ parse layout info """
    for line in lines:
        line = line.strip(" ")
        if line.startswith("|") and line.find("BSIZE") == -1:
            layoutsln = line
            break    
    layouts = layoutsln.split("|")
    def parse_blks(layout: str, parsed_layout: int):
        start = layout.find("(")
        end = layout.find(")")
        if start == -1 or end == -1:
            sys.stderr.write(ERROR + layout + " - 没有检查到对应的块数")
            exit(LAYOUT_FILE_ERR)
        if not layout[start + 1 : end].isdigit():
            if layout[start + 1 : end] != "*":
                sys.stderr.write(ERROR + layout + " - 请使用*, 而非" + layout[start + 1 : end])
                exit(LAYOUT_FILE_ERR)
            else:
                if parsed_layout < len(TOKENS_TOBE_CHECK):
                    sys.stderr.write(ERROR + layout + " - 块数不可解析为整数, 注意: 不能在还未完成Super, Inode Map和Data Map的解析之前使用*")
                    exit(LAYOUT_FILE_ERR)
                else:
                    return 0
        blks = int(layout[start + 1 : end])
        return blks
    cur_blks = 0
    parsed_layout = 0
    """ check consistency between layout and golden rules """
    for TOKEN in TOKENS_TOBE_CHECK:
        is_find = False
        for layout in layouts:
            layout = layout.strip(" ")
            if check_layout(layout, TOKEN):
                is_find = True
        if not is_find:
            if TOKEN == SUPER:
                sys.stderr.write(ERROR + os.path.basename(golden) + "中要检查super, 但" + os.path.basename(fslayout) + "中没有找到Super定义\n")
                exit(GOLDEN_LAYOUT_MISMATCH)
            if TOKEN == INODE_MAP:
                sys.stderr.write(ERROR + os.path.basename(golden) + "中要检查inode map, 但" + os.path.basename(fslayout) + "中没有找到inode map定义\n")
                exit(GOLDEN_LAYOUT_MISMATCH)
            if TOKEN == DATA_MAP:
                sys.stderr.write(ERROR + os.path.basename(golden) + "中要检查data map, 但" + os.path.basename(fslayout) + "中没有找到data map定义\n")
                exit(GOLDEN_LAYOUT_MISMATCH)
            if TOKEN == INODE:
                sys.stderr.write(ERROR + os.path.basename(golden) + "中要检查inode区, 但" + os.path.basename(fslayout) + "中没有找到inode区定义\n")
                exit(GOLDEN_LAYOUT_MISMATCH)
    """ parse layout info """
    for layout in layouts:
        layout = layout.strip(" ")
        if len(layout) != 0:
            blks = parse_blks(layout, parsed_layout)
            if check_layout(layout, SUPER):
                superblock_ofs = cur_blks
                superblock_blks = blks
                parsed_layout += 1
            elif check_layout(layout, INODE_MAP):
                inode_map_ofs = cur_blks
                inode_map_blks = blks
                parsed_layout += 1
            elif check_layout(layout, DATA_MAP):
                data_map_ofs = cur_blks
                data_map_blks = blks
                parsed_layout += 1
            elif check_layout(layout, INODE):
                inode_ofs = cur_blks
                inode_blks = blks
                parsed_layout += 1
            cur_blks += blks
    print("\n" + fslayout + " parsed result:")
    print("\tblock_size: %d" % block_size)
    print("\tsuperblock_ofs: %d" % superblock_ofs)
    print("\tsuperblock_blks: %d" % superblock_blks)
    if is_check_inode_map:
        print("\tinode_map_ofs: %d" % inode_map_ofs)
        print("\tinode_map_blks: %d" % inode_map_blks)
    if is_check_data_map:
        print("\tdata_map_ofs: %d" % data_map_ofs)
        print("\tdata_map_blks: %d" % data_map_blks)
    if is_check_data:
        print("\tinode_ofs: %d" % inode_ofs)
        print("\tinode_blks: %d" % inode_blks)


def check_map(f: TextIOWrapper, map_ofs: int, map_blks: int, golden_cnt: int):
    f.seek(map_ofs * block_size)
    bm = f.read(map_blks * block_size)
    valid_count = 0
    for i in range(0, int(map_blks * block_size / 4)):
        val = int.from_bytes(bm[i * 4 : i * 4 + 4], byteorder='little', signed=False)
        valid_count += bin(val).count("1")
    return [ valid_count == golden_cnt, valid_count, golden_cnt]

def check_data(f: TextIOWrapper, ino_ofs: int, ino_blks: int):
    res = True
    f.seek((ino_ofs + ino_blks) * block_size)
    bm = f.read(1 * block_size)
    valid_count = 0
    if all(byte == 0 for byte in bm):
        res = False
    else:
        res = True
    return res

with open(ddriver, "rb") as f:
    if is_check_inode_map:
        res1 = check_map(f, inode_map_ofs, inode_map_blks, valid_inode)
        if not res1[0]:
            sys.stderr.write("Inode位图错误, 期望值: %d个有效位, 实际值: %d个有效位" % (res1[2], res1[1]))
            exit(INODE_MAP_ERR)
    if is_check_data_map:
        res2 = check_map(f, data_map_ofs, data_map_blks, valid_data)
        if not res2[0]:
            sys.stderr.write("数据位图错误, 期望值: %d个有效位, 实际值: %d个有效位" % (res2[2], res2[1]))
            exit(DATA_MAP_ERR)
    if is_check_data:
        res3 = check_data(f, inode_ofs, inode_blks)
        if not res3:
            sys.stderr.write("数据写回错误, 没有写回到指定的数据区中")
            exit(DATA_ERR)
    exit(ERR_OK)

    