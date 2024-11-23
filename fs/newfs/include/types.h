#ifndef _TYPES_H_
#define _TYPES_H_

/******************************************************************************
* SECTION: Type def
*******************************************************************************/
typedef int          boolean;
typedef uint16_t     flag16;

typedef enum nfs_file_type {
    NFS_REG_FILE,
    NFS_DIR,
    //NFS_SYM_LINK  // 不用实现链接
} NFS_FILE_TYPE;

/******************************************************************************
* SECTION: Macro
*******************************************************************************/
#define TRUE                    1
#define FALSE                   0
#define UINT32_BITS             32
#define UINT8_BITS              8

#define NFS_MAGIC_NUM           0x52415453  
#define NFS_SUPER_OFS           0
#define NFS_ROOT_INO            0

#define NFS_ERROR_NONE          0
#define NFS_ERROR_ACCESS        EACCES
#define NFS_ERROR_SEEK          ESPIPE     
#define NFS_ERROR_ISDIR         EISDIR
#define NFS_ERROR_NOSPACE       ENOSPC
#define NFS_ERROR_EXISTS        EEXIST
#define NFS_ERROR_NOTFOUND      ENOENT
#define NFS_ERROR_UNSUPPORTED   ENXIO
#define NFS_ERROR_IO            EIO     /* Error Input/Output */
#define NFS_ERROR_INVAL         EINVAL  /* Invalid Args */

#define NFS_MAX_FILE_NAME       128
#define NFS_INODE_PER_FILE      1
#define NFS_DATA_PER_FILE       7
#define NFS_DEFAULT_PERM        0777

#define NFS_IOC_MAGIC           'S'
#define NFS_IOC_SEEK            _IO(NFS_IOC_MAGIC, 0)

#define NFS_FLAG_BUF_DIRTY      0x1
#define NFS_FLAG_BUF_OCCUPY     0x2

// 磁盘布局设计,一个逻辑块能放8个inode
#define NFS_INODE_PER_BLK       8     // 一个逻辑块能放8个inode
#define NFS_SUPER_BLKS          1
#define NFS_INODE_MAP_BLKS      1
#define NFS_DATA_MAP_BLKS       1
#define NFS_INODE_BLKS          64    // 4096/8/8(磁盘大小为4096个逻辑块，维护一个文件需要8个逻辑块即一个索引块+七个数据块)
#define NFS_DATA_BLKS           4029  // 4096-64-1-1-1

/******************************************************************************
* SECTION: Macro Function
*******************************************************************************/
#define NFS_IO_SZ()                     (nfs_super.sz_io)    // 512B
#define NFS_BLK_SZ()                    (nfs_super.sz_blks)  // 1KB
#define NFS_DISK_SZ()                   (nfs_super.sz_disk)  // 4MB
#define NFS_DRIVER()                    (nfs_super.fd)
#define NFS_BLKS_SZ(blks)               ((blks) * NFS_BLK_SZ())
#define NFS_DENTRY_PER_BLK()            (NFS_BLK_SZ() / sizeof(struct nfs_dentry))

// 向下取整以及向上取整
#define NFS_ROUND_DOWN(value, round)    ((value) % (round) == 0 ? (value) : ((value) / (round)) * (round))
#define NFS_ROUND_UP(value, round)      ((value) % (round) == 0 ? (value) : ((value) / (round) + 1) * (round))

// 复制文件名到某个dentry中
#define NFS_ASSIGN_FNAME(pnfs_dentry, _fname) memcpy(pnfs_dentry->fname, _fname, strlen(_fname))

// 计算inode和data的偏移量                                     
#define NFS_INO_OFS(ino)                (nfs_super.inode_offset + NFS_BLKS_SZ(ino))   // inode基地址初始偏移+前面的inode占用的空间
#define NFS_DATA_OFS(dno)               (nfs_super.data_offset + NFS_BLKS_SZ(dno))     // data基地址初始偏移+前面的data占用的空间

// 判断inode指向的是是目录还是普通文件
#define NFS_IS_DIR(pinode)              (pinode->dentry->ftype == NFS_DIR)
#define NFS_IS_REG(pinode)              (pinode->dentry->ftype == NFS_REG_FILE)

/******************************************************************************
* SECTION: FS Specific Structure - In memory structure
*******************************************************************************/
struct custom_options {
	const char*        device;
};

struct nfs_super {
    /* TODO: Define yourself */
    uint32_t           magic;             // 幻数
    int                fd;                // 文件描述符

    int                sz_io;             // 512B
    int                sz_blks;           // 1KB
    int                sz_disk;           // 4MB
    int                sz_usage;          // 已使用空间大小

    int                max_ino;           // 索引节点最大数量
    uint8_t*           map_inode;         // inode位图内存起点
    int                map_inode_blks;    // inode位图占用的逻辑块数量
    int                map_inode_offset;  // inode位图在磁盘中的偏移

    int                max_dno;           // 数据位图最大数量
    uint8_t*           map_data;          // data位图内存起点
    int                map_data_blks;     // data位图占用的逻辑块数量
    int                map_data_offset;   // data位图在磁盘中的偏移

    int                inode_offset;      // inode在磁盘中的偏移
    int                data_offset;       // data在磁盘中的偏移

    boolean            is_mounted;        // 是否挂载
    struct nfs_dentry* root_dentry;       // 根目录dentry
};

struct nfs_inode {
    /* TODO: Define yourself */
    uint32_t           ino;                               // 索引编号                         
    int                size;                              // 文件占用空间
    int                link;                              // 连接数默认为1(不考虑软链接和硬链接)
    int                block_pointer[NFS_DATA_PER_FILE];  // 数据块索引
    int                dir_cnt;                           // 如果是目录型文件，则代表有几个目录项
    struct nfs_dentry  *dentry;                           // 指向该inode的父dentry
    struct nfs_dentry  *dentrys;                          // 指向该inode的所有子dentry
    NFS_FILE_TYPE      ftype;                             // 文件类型
    uint8_t*           data[NFS_DATA_PER_FILE];           // 指向数据块的指针
    int                block_allocted;                    // 已分配数据块数量
};

struct nfs_dentry {
    /* TODO: Define yourself */
    char               fname[NFS_MAX_FILE_NAME];    // dentry指向的文件名
    struct nfs_dentry* parent;                      // 父目录的dentry            
    struct nfs_dentry* brother;                     // 兄弟dentry
    int                ino;                         // 指向的inode编号
    struct nfs_inode*  inode;                       // 指向的inode  
    NFS_FILE_TYPE      ftype;                       // 文件类型

};

// 生成新的dentry
static inline struct nfs_dentry* new_dentry(char * fname, NFS_FILE_TYPE ftype) {
    struct nfs_dentry * dentry = (struct nfs_dentry *)malloc(sizeof(struct nfs_dentry));
    memset(dentry, 0, sizeof(struct nfs_dentry));
    NFS_ASSIGN_FNAME(dentry, fname);
    dentry->ftype   = ftype;
    dentry->ino     = -1;
    dentry->inode   = NULL;
    dentry->parent  = NULL;
    dentry->brother = NULL;  
    return dentry;                                          
}

/******************************************************************************
* SECTION: FS Specific Structure - To Disk structure
*******************************************************************************/
struct nfs_super_d {
    uint32_t           magic;             // 幻数
    int                sz_usage;          // 已使用空间大小

    int                max_ino;           // 索引节点最大数量
    int                map_inode_blks;    // inode位图占用的逻辑块数量
    int                map_inode_offset;  // inode位图在磁盘中的偏移

    int                max_dno;           // 数据位图最大数量
    int                map_data_blks;     // data位图占用的逻辑块数量
    int                map_data_offset;   // data位图在磁盘中的偏移

    int                inode_offset;      // inode在磁盘中的偏移
    int                data_offset;       // data在磁盘中的偏移
};

struct nfs_inode_d {
    uint32_t           ino;                               // 索引编号                         
    int                size;                              // 文件占用空间(用了多少个逻辑块) 
    int                link;                              // 连接数默认为1(不考虑软链接和硬链接)
    int                block_pointer[NFS_DATA_PER_FILE];  // 数据块索引
    int                dir_cnt;                           // 如果是目录型文件，则代表有几个目录项
    NFS_FILE_TYPE      ftype;                             // 文件类型
    int                block_allocted;                    // 已分配数据块数量
};

struct nfs_dentry_d {
    char               fname[NFS_MAX_FILE_NAME];    // dentry指向的文件名
    int                ino;                         // 指向的inode编号
    NFS_FILE_TYPE      ftype;                       // 文件类型
};

#endif /* _TYPES_H_ */