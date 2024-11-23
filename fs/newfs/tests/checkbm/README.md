# 位图检查原理

## 1. 文件组成

- `check_bm.py`: 

  - 文件位置：位于`./tests/checkbm`下
  - 文件功能：该脚本为核心检查执行脚本，在创建的文件系统根目录 (以**SFS**为例，该路径就是`user-land-filesystem/fs/simplefs/`) 下可以通过下述命令调用`check_bm.py`

  ```shell
  python3 ./tests/checkbm/checkbm.py -l ./include/fs.layout -r ./tests/checkbm/golden.json
  ```

- `golden.json`: 

  - 文件位置：位于`./tests/checkbm`下
  - 文件功能：作为`check_bm.py`的输入，是文件系统位图的**检查规则**

- `fs.layout`:

  - 文件位置：位于`./include/`下
  - 文件功能：作为`check_bm.py`的输入，定义了欲检查的文件系统的**布局规则**，详请自行参考

## 2. check_bm.py运行原理

### 2.1 参数含义

该脚本接受两个输入，具体帮助如下:

```shell
$ python checkbm.py --help

usage: checkbm.py [-h] [-l LAYOUT] [-r RULES]

optional arguments:
  -h, --help            show this help message and exit
  -l LAYOUT, --layout LAYOUT
                        absolute path of .layout file
  -r RULES, --rules RULES
                        absolute path of golden rule json file
```

即：

- `-l`或`--layout`指定`fs.layout`文件的绝对路径
- `-r`或`--rules`指定`golden.json`文件的绝对路径

### 2.2 运行原理

`checkbm.py`的运行流程如下：

1. 识别`ddriver`路径，**后续检查都将发生在`ddriver`磁盘布局中**

2. 解析`golden.json`文件，确定检测对象及对象规则。目前仅支持检测`super`、`data_map`以及`inode_map`

   - `super`：检测超级块的有无
   - `data_map`：检测数据位图。使用`valid_data`指定有效数据块数量
   - `inode_map`：检测`inode`位图。使用`valid_inode`指定有效文件 (`inode`) 数量

   下面是**SFS**的`golden.json`示例：

   ```json
   {
       "checks": [
           "super",
           "inode_map"
       ],
       "valid_inode": 2
   }
   ```

   该规则代表检查SFS中的`super`与`inode_map`，其中，保证`inode_map`只有两个有效位，即目前仅分配2个`inode`。如果

3. 解析`fs.layout`文件，确定`ddriver`磁盘布局。为了验证`golden.json`规则，`check_bm.py`需要知道你的文件系统`super`块在哪里，`inode_map`在哪里，`data_map`在哪里以及他们的大小等。`fs.layout`的定义请自行查看文件内部说明。

4. 检查各项指标

   - 检查`super`。用户需要在`fs.layout`中定义`super`布局，否则该项检查失败；
   - 检查`inode_map`。我们通过`fs.layout`计算出`inode_map`在`ddriver`上位置及其大小，并便利整个`map`，计算其中1的个数，如果不等于`golden.json`中定义的`valid_inode`数，则检查失败；
   - 检查`data_map`。与检查`inode_map`的操作一致，不做赘述。

5. 以上任意检查失败，`check_bm.py`会返回相应的错误，大家依错自行查改即可。

## 3. Hint：布局调试方法

文件系统的布局是文件系统的**核心骨架**。理解了介质上的布局就基本上理解了整个文件系统的工作原理。因此，除了避免大家直接抄袭**SFS**，`check_bm.py`更大的一个目的是强制大家去理解介质布局。

为了快速查看介质布局，我们强烈推荐`VSCode` +`Hex Editor` + `ddriver.sh`。其中`VSCode` +`Hex Editor`用于查看布局的二进制结构；`ddriver.sh`用于快速操作介质，例如：复制介质到当前目录，擦除介质等。

```shell
====================================================================
]]]]]]]]    ]]]]]]]]             
]      ]]   ]      ]]   ]   ]]   ]]   ]]    ]]    ]-------]   ]   ]]   
]       ]   ]       ]   ] ]]     ]]    ]]  ]]     ]_______/   ] ]]     
]      ]]   ]      ]]   ]]       ]]     ]  ]      \           ]]      
]]]]]]]]    ]]]]]]]]    ]]       ]]      ]]        ]]]]]]]]   ]]  
====================================================================
用法: ddriver [options]
options: 
-i [k|u]      安装ddriver: [k] - kernel / [u] - user"
-t            测试ddriver[请忽略]
-d            导出ddriver至当前工作目录[PWD]
-r            擦除ddriver
-l            显示ddriver的Log
-v            显示ddriver的类型[内核模块 / 用户静态链接库]
-h            打印本帮助菜单
====================================================================
```

一个很常用的命令是：

```shell
ddriver -d
```

即，将`ddriver`磁盘布局拷贝到当前目录下。此时使用`VSCode`+ `Hex Editor`查看布局情况即可。如果布局情况与理论布局不同，那就要看看是不是有逻辑错误，例如：偏移写错等。
