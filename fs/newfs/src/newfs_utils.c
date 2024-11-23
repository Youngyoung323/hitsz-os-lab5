#include "../include/newfs.h"

struct nfs_super      nfs_super; 
struct custom_options nfs_options;

/**
 * @brief 获取文件名
 * 
 * @param path 
 * @return char* 
 */
char* nfs_get_fname(const char* path) {
    char ch = '/';
    // strrchr函数返回一个指针，指向字符串中最后一个出现的字符ch的位置(逆向搜索)
    char *q = strrchr(path, ch) + 1;
    return q;
}

/**
 * @brief 计算路径的层级
 * exm: /av/c/d/f
 * -> lvl = 4
 * @param path 
 * @return int 
 */
int nfs_calc_lvl(const char * path) {
    char* str = path;
    int   lvl = 0;
    // 根目录
    if (strcmp(path, "/") == 0) {
        return lvl;
    }
    while (*str != NULL) {
        if (*str == '/') {
            lvl++;
        }
        str++;
    }
    return lvl;
}

/**
 * @brief 驱动读
 * 
 * @param offset 
 * @param out_content 
 * @param size 
 * @return int 
 */
int nfs_driver_read(int offset, uint8_t *out_content, int size) {
    // 对齐，按一个逻辑块大小(两个IO大小)进行读写
    int      offset_aligned = NFS_ROUND_DOWN(offset, NFS_BLK_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = NFS_ROUND_UP((size + bias), NFS_BLK_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    
    // 磁盘头定位到down位置
    ddriver_seek(NFS_DRIVER(), offset_aligned, SEEK_SET);
    // 按照IO大小进行读，从down开始读size_aligned大小的内容
    while (size_aligned != 0)
    {
        ddriver_read(NFS_DRIVER(), cur, NFS_IO_SZ());
        cur          += NFS_IO_SZ();
        size_aligned -= NFS_IO_SZ();   
    }
    // 从down+bias开始拷贝size大小的内容到out_content
    memcpy(out_content, temp_content + bias, size);
    free(temp_content);
    return NFS_ERROR_NONE;
}

/**
 * @brief 驱动写
 * 
 * @param offset 
 * @param in_content 
 * @param size 
 * @return int 
 */
int nfs_driver_write(int offset, uint8_t *in_content, int size) {
    // 对齐，按一个逻辑块大小(两个IO大小)进行读写
    int      offset_aligned = NFS_ROUND_DOWN(offset, NFS_BLK_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = NFS_ROUND_UP((size + bias), NFS_BLK_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    // 读出被写磁盘块到内存
    nfs_driver_read(offset_aligned, temp_content, size_aligned);
    // 从down+bias开始覆盖size大小的内容
    memcpy(temp_content + bias, in_content, size);
    // 磁盘头定位到down
    ddriver_seek(NFS_DRIVER(), offset_aligned, SEEK_SET);

    // 内容在内存中修改后写回磁盘
    while (size_aligned != 0)
    {
        ddriver_write(NFS_DRIVER(), cur, NFS_IO_SZ());
        cur          += NFS_IO_SZ();
        size_aligned -= NFS_IO_SZ();   
    }

    free(temp_content);
    return NFS_ERROR_NONE;
}

/**
 * @brief 将dentry插入到inode中，采用头插法
 * 
 * @param inode 
 * @param dentry 
 * @return int 
 */
int nfs_alloc_dentry(struct nfs_inode* inode, struct nfs_dentry* dentry) {
    if (inode->dentrys == NULL) {
        inode->dentrys = dentry;
    }
    else {
        dentry->brother = inode->dentrys;
        inode->dentrys = dentry;
    }
    inode->dir_cnt++;
    inode->size += sizeof(struct nfs_dentry);

    // 判断是否需要重新分配一个数据块
    if (inode->dir_cnt % NFS_DENTRY_PER_BLK() == 1) {
        inode->block_pointer[inode->block_allocted] = nfs_alloc_data();
        inode->block_allocted++;
    }
    return inode->dir_cnt;
}

/**
 * @brief 分配一个inode，占用位图
 * 
 * @param dentry 该dentry指向分配的inode
 * @return nfs_inode
 */
struct nfs_inode* nfs_alloc_inode(struct nfs_dentry * dentry) {
    struct nfs_inode* inode;
    int byte_cursor   = 0; 
    int bit_cursor    = 0; 
    int ino_cursor    = 0;  
    boolean is_find_free_entry   = FALSE;

    inode = (struct nfs_inode*)malloc(sizeof(struct nfs_inode));

    // 先按字节寻找空闲的inode位图
    for (byte_cursor = 0; byte_cursor < NFS_BLKS_SZ(nfs_super.map_inode_blks); byte_cursor++)
    {
        // 再在该字节中遍历8个bit寻找空闲的inode位图
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if((nfs_super.map_inode[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                // 当前ino_cursor位置空闲 
                nfs_super.map_inode[byte_cursor] |= (0x1 << bit_cursor);
                is_find_free_entry = TRUE;           
                break;
            }
            ino_cursor++;
        }
        if (is_find_free_entry) {
            break;
        }
    }

    /*// 先按字节寻找空闲的inode位图
    for (byte_cursor = 0; byte_cursor < NFS_BLKS_SZ(nfs_super.map_inode_blks); byte_cursor++)
    {
        // 再在该字节中遍历8个bit寻找空闲的inode位图
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if((nfs_super.map_data[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                // 当前dno_cursor位置空闲 
                nfs_super.map_data[byte_cursor] |= (0x1 << bit_cursor);
                inode->block_pointer[data_blks_num++] = dno_cursor;
                // 分配完足够数量的位图再退出
                if (data_blks_num == NFS_DATA_PER_FILE) {
                    is_find_enough_entry = TRUE;          
                    break;
                }
            }
            dno_cursor++;
        }
        if (is_find_enough_entry) {
            break;
        }
    }*/

    // 上面的实现有问题,应该是预先分配一个数据块,等到这个数据块不够用的时候再分配新的数据块
    // 具体实现应该放在alloc_dentry中，新建一个alloc_data函数辅助实现

    // 未找到空闲结点
    if (!is_find_free_entry || ino_cursor >= nfs_super.max_ino)
        return -NFS_ERROR_NOSPACE;

    // 为目录项分配inode节点
    inode->ino  = ino_cursor; 
    inode->size = 0;
    inode->dir_cnt = 0;
    inode->block_allocted = 0;
    inode->dentrys = NULL;

    // dentry指向分配的inode 
    dentry->inode = inode;
    dentry->ino   = inode->ino;
    // inode指回dentry 
    inode->dentry = dentry;
    
    // 文件类型需要分配空间,目录项已经在dentrys里,普通文件不要求额外分配数据块的操作,一次性分配完就好了
    if (NFS_IS_REG(inode)) {
        for (int i = 0; i < NFS_DATA_PER_FILE; i++) {
            inode->data[i] = (uint8_t *)malloc(NFS_BLK_SZ());
        }
    }

    return inode;
}

/**
 * @brief 额外分配一个数据块
 * @return 分配的数据块号
 */
 int nfs_alloc_data() {
    int byte_cursor       = 0; 
    int bit_cursor        = 0;   
    int dno_cursor        = 0; 
    int is_find_free_data = 0;

    for (byte_cursor = 0; byte_cursor < NFS_BLKS_SZ(nfs_super.map_data_blks); byte_cursor++) {
        // 再在该字节中遍历8个bit寻找空闲的inode位图
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if((nfs_super.map_data[byte_cursor] & (0x1 << bit_cursor)) == 0) {
                // 当前dno_cursor位置空闲
                nfs_super.map_data[byte_cursor] |= (0x1 << bit_cursor);
                is_find_free_data = 1;
                break;
            }
            dno_cursor++;
        }
        if (is_find_free_data) {
            break;
        }
    }

    if (!is_find_free_data || dno_cursor >= nfs_super.max_dno) {
        return -NFS_ERROR_NOSPACE;
    }

    return dno_cursor;
 }

/**
 * @brief 将内存inode及其下方结构全部刷回磁盘
 * 
 * @param inode 
 * @return int 
 */
int nfs_sync_inode(struct nfs_inode * inode) {
    struct nfs_inode_d  inode_d;
    struct nfs_dentry*  dentry_cursor;
    struct nfs_dentry_d dentry_d;
    int offset;
    int ino             = inode->ino;

    // 把inode的内容拷贝到inode_d中
    inode_d.ino            = ino;
    inode_d.size           = inode->size;
    inode_d.ftype          = inode->dentry->ftype;
    inode_d.dir_cnt        = inode->dir_cnt;
    inode_d.block_allocted = inode->block_allocted;
    for (int i = 0; i < NFS_DATA_PER_FILE; i++) {
        inode_d.block_pointer[i] = inode->block_pointer[i];
    }
    
    // 将inode_d刷回磁盘
    if (nfs_driver_write(NFS_INO_OFS(ino), (uint8_t *)&inode_d, 
                     sizeof(struct nfs_inode_d)) != NFS_ERROR_NONE) {
        NFS_DBG("[%s] io error\n", __func__);
        return -NFS_ERROR_IO;
    }

    // 刷回inode的数据块
    if (NFS_IS_DIR(inode)) { // 如果当前inode是目录，那么数据是目录项，且目录项的inode也要写回                         
        dentry_cursor     = inode->dentrys;
        int data_blks_num = 0;
        // 要将dentry的内容刷回磁盘，有七个数据块
        while ((dentry_cursor != NULL) && (data_blks_num < inode->block_allocted)) {
            offset = NFS_DATA_OFS(inode->block_pointer[data_blks_num]);
            while ((dentry_cursor != NULL) && (offset + sizeof(struct nfs_dentry_d) < NFS_DATA_OFS(inode->block_pointer[data_blks_num] + 1))) {
                // dentry的内容复制到dentry_d中
                memcpy(dentry_d.fname, dentry_cursor->fname, NFS_MAX_FILE_NAME);
                dentry_d.ftype = dentry_cursor->ftype;
                dentry_d.ino = dentry_cursor->ino;
                // dentry_d的内容刷回磁盘
                if (nfs_driver_write(offset, (uint8_t *)&dentry_d, sizeof(struct nfs_dentry_d))!= NFS_ERROR_NONE) {
                    NFS_DBG("[%s] io error\n", __func__);
                    return -NFS_ERROR_IO;
                }

                // 递归处理目录项的内容
                if (dentry_cursor->inode != NULL) {
                    nfs_sync_inode(dentry_cursor->inode);
                }

                dentry_cursor  = dentry_cursor->brother;
                offset += sizeof(struct nfs_dentry_d);
            }
            data_blks_num++;
        }
    }
    // 如果当前inode是文件，那么数据是文件内容，直接写即可 
    else if (NFS_IS_REG(inode)) { 
        for (int i = 0; i < inode->block_allocted; i++) {
            if (nfs_driver_write(NFS_DATA_OFS(inode->block_pointer[i]), inode->data[i], NFS_BLKS_SZ(NFS_DATA_PER_FILE)) != NFS_ERROR_NONE) {
                NFS_DBG("[%s] io error\n", __func__);
                return -NFS_ERROR_IO;
            }
        }
    }

    return NFS_ERROR_NONE;
}

/**
 * @brief 
 * 
 * @param dentry dentry指向ino，读取该inode
 * @param ino inode唯一编号
 * @return struct nfs_inode* 
 */
struct nfs_inode* nfs_read_inode(struct nfs_dentry * dentry, int ino) {
    struct nfs_inode* inode = (struct nfs_inode*)malloc(sizeof(struct nfs_inode));
    struct nfs_inode_d inode_d;
    struct nfs_dentry* sub_dentry;
    struct nfs_dentry_d dentry_d;
    int    dir_cnt = 0;

    // 从磁盘读索引结点 
    if (nfs_driver_read(NFS_INO_OFS(ino), (uint8_t *)&inode_d, 
                        sizeof(struct nfs_inode_d)) != NFS_ERROR_NONE) {
        NFS_DBG("[%s] io error\n", __func__);
        return NULL;                    
    }
    // 根据inode_d的内容初始化inode
    inode->dir_cnt = 0;
    inode->ino = inode_d.ino;
    inode->size = inode_d.size;
    inode->dentry = dentry;
    inode->dentrys = NULL;
    inode->block_allocted = inode_d.block_allocted;
    for (int i = 0; i < NFS_DATA_PER_FILE; i++) {
        inode->block_pointer[i] = inode_d.block_pointer[i];
    }

    // 内存中的inode的数据或子目录项部分也需要读出 
    if (NFS_IS_DIR(inode)) {
        dir_cnt           = inode_d.dir_cnt;
        int data_blks_num = 0;
        int offset;

        // 对所有的目录项都进行处理(先分数据块处理)
        while((dir_cnt > 0) && (data_blks_num < NFS_DATA_PER_FILE)){
            offset = NFS_DATA_OFS(inode->block_pointer[data_blks_num]);

            // 再分单独的目录项进行处理
            while((dir_cnt > 0) && (offset + sizeof(struct nfs_dentry_d) < NFS_DATA_OFS(inode->block_pointer[data_blks_num] + 1))){
                if (nfs_driver_read(offset, (uint8_t *)&dentry_d, sizeof(struct nfs_dentry_d)) != NFS_ERROR_NONE){
                    NFS_DBG("[%s] io error\n", __func__);
                    return NULL;  
                }
                
                // 用从磁盘中读出的dentry_d更新内存中的sub_dentry 
                sub_dentry = new_dentry(dentry_d.fname, dentry_d.ftype);
                sub_dentry->parent = inode->dentry;
                sub_dentry->ino    = dentry_d.ino; 
                nfs_alloc_dentry(inode, sub_dentry);

                offset += sizeof(struct nfs_dentry_d);
                dir_cnt--;
            }
            data_blks_num++;
        }
    }
    // inode是普通文件则直接读取数据块
    else if (NFS_IS_REG(inode)) {
        for (int i = 0; i < NFS_DATA_PER_FILE; i++) {
            inode->data[i] = (uint8_t *)malloc(NFS_BLKS_SZ(NFS_DATA_PER_FILE));
            if (nfs_driver_read(NFS_DATA_OFS(inode->block_pointer[i]), (uint8_t *)inode->data[i], 
                            NFS_BLKS_SZ(NFS_DATA_PER_FILE)) != NFS_ERROR_NONE) {
                NFS_DBG("[%s] io error\n", __func__);
                return NULL;                    
            }
        }
    }

    return inode;
}

/**
 * @brief 
 * 
 * @param inode 
 * @param dir [0...]
 * @return struct nfs_dentry* 
 */
struct nfs_dentry* nfs_get_dentry(struct nfs_inode * inode, int dir) {
    struct nfs_dentry* dentry_cursor = inode->dentrys;
    int    cnt = 0;
    // 获取某个inode的第dir个目录项
    while (dentry_cursor)
    {
        if (dir == cnt) {
            return dentry_cursor;
        }
        cnt++;
        dentry_cursor = dentry_cursor->brother;
    }
    return NULL;
}

/**
 * @brief 查找文件或目录
 * path: /qwe/ad  total_lvl = 2,
 *      1) find /'s inode       lvl = 1
 *      2) find qwe's dentry 
 *      3) find qwe's inode     lvl = 2
 *      4) find ad's dentry
 *
 * path: /qwe     total_lvl = 1,
 *      1) find /'s inode       lvl = 1
 *      2) find qwe's dentry
 *  
 * 
 * 如果能查找到，返回该目录项
 * 如果查找不到，返回的是上一个有效的路径
 * 
 * path: /a/b/c
 *      1) find /'s inode     lvl = 1
 *      2) find a's dentry 
 *      3) find a's inode     lvl = 2
 *      4) find b's dentry    如果此时找不到了，is_find=FALSE且返回的是a的inode对应的dentry
 * 
 * @param path 
 * @return struct nfs_dentry* 
 */
struct nfs_dentry* nfs_lookup(const char * path, boolean* is_find, boolean* is_root) {
    struct nfs_dentry* dentry_cursor = nfs_super.root_dentry;
    struct nfs_dentry* dentry_ret = NULL;
    struct nfs_inode*  inode; 
    int   total_lvl = nfs_calc_lvl(path);
    int   lvl       = 0;
    boolean is_hit;
    char* fname     = NULL;
    char* path_cpy  = (char*)malloc(sizeof(path));
    *is_root        = FALSE;
    strcpy(path_cpy, path);

    // 根目录 
    if (total_lvl == 0) {                           
        *is_find = TRUE;
        *is_root = TRUE;
        dentry_ret = nfs_super.root_dentry;
    }

    // strtok用来分割路径以此获得最外层文件名
    fname = strtok(path_cpy, "/");       
    while (fname)
    {   
        lvl++;
        // Cache机制,如果当前dentry的inode为空则从磁盘读出来
        if (dentry_cursor->inode == NULL) {           
            nfs_read_inode(dentry_cursor, dentry_cursor->ino);
        }

        inode = dentry_cursor->inode;

        // 还没到对应层数就查到普通文件，无法继续往下查询
        if (NFS_IS_REG(inode) && lvl < total_lvl) {
            NFS_DBG("[%s] not a dir\n", __func__);
            dentry_ret = inode->dentry;
            break;
        }

        // 当前inode对应的是一个目录
        if (NFS_IS_DIR(inode)) {
            dentry_cursor = inode->dentrys;
            is_hit        = FALSE;

            // 遍历子目录项找到对应文件名称的dentry
            while (dentry_cursor)   
            {
                if (memcmp(dentry_cursor->fname, fname, strlen(fname)) == 0) {
                    is_hit = TRUE;
                    break;
                }
                dentry_cursor = dentry_cursor->brother;
            }
            
            // 没有找到对应文件夹名称的目录项则返回最后找到的文件夹的dentry
            if (!is_hit) {
                *is_find = FALSE;
                NFS_DBG("[%s] not found %s\n", __func__, fname);
                dentry_ret = inode->dentry;
                break;
            }

            if (is_hit && lvl == total_lvl) {
                *is_find = TRUE;
                dentry_ret = dentry_cursor;
                break;
            }
        }
        // 继续使用strtok获取下一层文件的名称
        fname = strtok(NULL, "/"); 
    }

    // 再出确保dentry_cursor对应的inode不为空
    if (dentry_ret->inode == NULL) {
        dentry_ret->inode = nfs_read_inode(dentry_ret, dentry_ret->ino);
    }
    
    return dentry_ret;
}

/**
 * @brief 挂载sfs, Layout 如下
 * 
 * Layout
 * 每8个Inode占用一个Blk
 * @param options 
 * @return int 
 */
int nfs_mount(struct custom_options options){
    int                 ret = NFS_ERROR_NONE;
    int                 driver_fd;
    struct nfs_super_d  nfs_super_d; 
    struct nfs_dentry*  root_dentry;
    struct nfs_inode*   root_inode;

    int                 inode_num;
    int                 map_inode_blks;
    int                 data_num;
    int                 map_data_blks;
    
    int                 super_blks;
    boolean             is_init = FALSE;

    nfs_super.is_mounted = FALSE;
    driver_fd = ddriver_open(options.device);

    if (driver_fd < 0) {
        return driver_fd;
    }

    // 向超级块中写入相关信息
    nfs_super.fd = driver_fd;
    ddriver_ioctl(NFS_DRIVER(), IOC_REQ_DEVICE_SIZE,  &nfs_super.sz_disk);
    ddriver_ioctl(NFS_DRIVER(), IOC_REQ_DEVICE_IO_SZ, &nfs_super.sz_io);
    nfs_super.sz_blks = 2 * nfs_super.sz_io;  // 两个IO大小
    // 新建根目录
    root_dentry = new_dentry("/", NFS_DIR);

    // 读取磁盘超级块内容1
    if (nfs_driver_read(NFS_SUPER_OFS, (uint8_t *)(&nfs_super_d), sizeof(struct nfs_super_d)) != NFS_ERROR_NONE) {
        return -NFS_ERROR_IO;
    }   
    
    // 根据磁盘超级块的幻数判断是否是第一次挂载
    if (nfs_super_d.magic != NFS_MAGIC_NUM) {    
        // 计算各部分的大小
        // 宏定义在type.h中
        super_blks      = NFS_SUPER_BLKS;
        map_inode_blks  = NFS_INODE_MAP_BLKS;
        map_data_blks   = NFS_DATA_MAP_BLKS;
        inode_num       = NFS_INODE_BLKS;
        data_num        = NFS_DATA_BLKS;

        // 布局layout 
        nfs_super.max_ino               = inode_num;
        nfs_super.max_dno               = data_num;
        nfs_super_d.map_inode_blks      = map_inode_blks; 
        nfs_super_d.map_data_blks       = map_data_blks; 
        nfs_super_d.map_inode_offset    = NFS_SUPER_OFS + NFS_BLKS_SZ(super_blks);
        nfs_super_d.map_data_offset     = nfs_super_d.map_inode_offset + NFS_BLKS_SZ(map_inode_blks);
        nfs_super_d.inode_offset        = nfs_super_d.map_data_offset + NFS_BLKS_SZ(map_data_blks);
        nfs_super_d.data_offset         = nfs_super_d.inode_offset + NFS_BLKS_SZ(inode_num);

        nfs_super_d.sz_usage            = 0;
        nfs_super_d.magic               = NFS_MAGIC_NUM;

        is_init = TRUE;
    }

    // 建立 in-memory 结构 
    // 初始化超级块
    nfs_super.sz_usage   = nfs_super_d.sz_usage; 

    // 建立inode位图    
    nfs_super.map_inode         = (uint8_t *)malloc(NFS_BLKS_SZ(nfs_super_d.map_inode_blks));
    nfs_super.map_inode_blks    = nfs_super_d.map_inode_blks;
    nfs_super.map_inode_offset  = nfs_super_d.map_inode_offset;
    nfs_super.inode_offset      = nfs_super_d.inode_offset;

    // 建立数据块位图
    nfs_super.map_data          = (uint8_t *)malloc(NFS_BLKS_SZ(nfs_super_d.map_data_blks));
    nfs_super.map_data_blks     = nfs_super_d.map_data_blks;
    nfs_super.map_data_offset   = nfs_super_d.map_data_offset;
    nfs_super.data_offset       = nfs_super_d.data_offset;

    // 从磁盘中读取索引位图
    if (nfs_driver_read(nfs_super_d.map_inode_offset, (uint8_t *)(nfs_super.map_inode), 
                        NFS_BLKS_SZ(nfs_super_d.map_inode_blks)) != NFS_ERROR_NONE) {
        return -NFS_ERROR_IO;
    }

    // 从磁盘中读取数据位图
    if (nfs_driver_read(nfs_super_d.map_data_offset, (uint8_t *)(nfs_super.map_data), 
                        NFS_BLKS_SZ(nfs_super_d.map_data_blks)) != NFS_ERROR_NONE) {
        return -NFS_ERROR_IO;
    }

    // 分配根节点 
    if (is_init) {                                    
        root_inode = nfs_alloc_inode(root_dentry);
        nfs_sync_inode(root_inode);
    }
    
    root_inode            = nfs_read_inode(root_dentry, NFS_ROOT_INO);
    root_dentry->inode    = root_inode;
    nfs_super.root_dentry = root_dentry;
    nfs_super.is_mounted  = TRUE;

    return ret;
}

/**
 * @brief 卸载nfs
 * 
 * @return int 
 */
int nfs_umount() {
    struct nfs_super_d  nfs_super_d; 

    // 没有挂载直接报错
    if (!nfs_super.is_mounted) {
        return NFS_ERROR_NONE;
    }

    // 从根节点向下刷写节点 
    nfs_sync_inode(nfs_super.root_dentry->inode);     

    // 将内存中的超级块刷回磁盘                              
    nfs_super_d.magic               = NFS_MAGIC_NUM;
    nfs_super_d.sz_usage            = nfs_super.sz_usage;

    nfs_super_d.map_inode_blks      = nfs_super.map_inode_blks;
    nfs_super_d.map_inode_offset    = nfs_super.map_inode_offset;
    nfs_super_d.inode_offset        = nfs_super.inode_offset;

    nfs_super_d.map_data_blks       = nfs_super.map_data_blks;
    nfs_super_d.map_data_offset     = nfs_super.map_data_offset;
    nfs_super_d.data_offset         = nfs_super.data_offset;
    
    if (nfs_driver_write(NFS_SUPER_OFS, (uint8_t *)&nfs_super_d, 
                     sizeof(struct nfs_super_d)) != NFS_ERROR_NONE) {
        return -NFS_ERROR_IO;
    }

    // 将索引位图刷回磁盘 
    if (nfs_driver_write(nfs_super_d.map_inode_offset, (uint8_t *)(nfs_super.map_inode), 
                         NFS_BLKS_SZ(nfs_super_d.map_inode_blks)) != NFS_ERROR_NONE) {
        return -NFS_ERROR_IO;
    }

    // 将数据位图刷回磁盘
    if (nfs_driver_write(nfs_super_d.map_data_offset, (uint8_t *)(nfs_super.map_data), 
                         NFS_BLKS_SZ(nfs_super_d.map_data_blks)) != NFS_ERROR_NONE) {
        return -NFS_ERROR_IO;
    }

    // 释放内存中的位图
    free(nfs_super.map_inode);
    free(nfs_super.map_data);

    // 关闭驱动 
    ddriver_close(NFS_DRIVER());

    return NFS_ERROR_NONE;
}