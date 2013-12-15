#include "inode_manager.h"

// disk layer -----------------------------------------

disk::disk()
{
  bzero(blocks, sizeof(blocks));
}

void
disk::read_block(blockid_t id, char *buf)
{
  if (id < 0 || id >= BLOCK_NUM || buf == NULL)
    return;

  memcpy(buf, blocks[id], BLOCK_SIZE);
}

void
disk::write_block(blockid_t id, const char *buf)
{
  if (id < 0 || id >= BLOCK_NUM || buf == NULL)
    return;

  memcpy(blocks[id], buf, BLOCK_SIZE);
}

// block layer -----------------------------------------

// Allocate a free disk block.
blockid_t
block_manager::alloc_block()
{
    int skip = 1 + 1 + sb.nblocks / BPB + INODE_NUM / IPB;
    for(int i=0; i<sb.nblocks; i+=BPB){
        char buf[BLOCK_SIZE];
        read_block(BBLOCK(i), buf);
        for(int j=0; j<BPB; j++){
            if(i + j < skip)continue;
            int test = 1 << (j%8);                    
            if((buf[j/8] & test) == 0){                 
                buf[j/8] |= (test);         
                write_block(BBLOCK(i), buf);
                char none[BLOCK_SIZE];
                memset(none, 0, sizeof(char) * BLOCK_SIZE);
                write_block(i + j, none);
                return i + j;
            }
        }
    }
    printf("\tbm: can't alloc block! \n");
    exit(0);
}

void
block_manager::free_block(uint32_t id)
{
    uint32_t block_num = BBLOCK(id);
    char buf[BLOCK_SIZE];
    read_block(block_num, buf);
    uint32_t bit_num = id - (block_num - BBLOCK(0)) * BPB;
    char test = 1 << (bit_num%8);                       
    if(buf[bit_num/8]&test == 0){
        printf("\tbm: this block has been free! \n");
        return;
    }
    buf[bit_num/8] &= (~test);
    write_block(block_num, buf);
  return;
}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager()
{
  d = new disk();

  // format the disk
  sb.size = BLOCK_SIZE * BLOCK_NUM;
  sb.nblocks = BLOCK_NUM;
  sb.ninodes = INODE_NUM;

}

void
block_manager::read_block(uint32_t id, char *buf)
{
  d->read_block(id, buf);
}

void
block_manager::write_block(uint32_t id, const char *buf)
{
  d->write_block(id, buf);
}

// inode layer -----------------------------------------
inode_manager::inode_manager()
{
  bm = new block_manager();
  uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
  if (root_dir != 1) {
    printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
    exit(0);
  }
}


/* Create a new file.
 * Return its inum. */
uint32_t
inode_manager::alloc_inode(uint32_t type)
{
   struct inode *ino_disk;
   for(int i=1; i<bm->sb.ninodes; i++){
        char buf[BLOCK_SIZE];
        bm->read_block(IBLOCK(i, bm->sb.nblocks), buf);
        ino_disk = (struct inode*)buf + i%IPB;
        if(ino_disk->type == 0){                            
            memset(ino_disk, 0, sizeof(struct inode));
            ino_disk->type = type;
            ino_disk->mtime = time(NULL);
            bm->write_block(IBLOCK(i, bm->sb.nblocks), buf);
            return i;
        }
   }
   printf("\tim: can't alloc inode! \n");
   exit(0);
}

void
inode_manager::free_inode(uint32_t inum)
{
   struct inode *ino_disk;
   char buf[BLOCK_SIZE];
   bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
   ino_disk = (struct inode*)buf + inum%IPB;
   if(ino_disk->type == 0){
        printf("\tim: this inode has been free! \n");
        return;
   }
   memset(ino_disk, 0, sizeof(struct inode));
   bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}


/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode*
inode_manager::get_inode(uint32_t inum)
{
  struct inode *ino, *ino_disk;
  char buf[BLOCK_SIZE];

  printf("\tim: get_inode %d\n", inum);

  if (inum < 0 || inum >= INODE_NUM) {
    printf("\tim: inum out of range\n");
    return NULL;
  }

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  // printf("%s:%d\n", __FILE__, __LINE__);

  ino_disk = (struct inode*)buf + inum%IPB;
  if (ino_disk->type == 0) {
    printf("\tim: inode not exist\n");
    return NULL;
  }

  ino = (struct inode*)malloc(sizeof(struct inode));
  *ino = *ino_disk;

  return ino;
}

void
inode_manager::put_inode(uint32_t inum, struct inode *ino)
{
  char buf[BLOCK_SIZE];
  struct inode *ino_disk;

  printf("\tim: put_inode %d\n", inum);
  if (ino == NULL)
    return;

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  ino_disk = (struct inode*)buf + inum%IPB;
  *ino_disk = *ino;
  ino_disk->ctime = time(NULL);
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

#define MIN(a,b) ((a)<(b) ? (a) : (b))

/* Get all the data of a file by inum.
 * Return alloced data, should be freed by caller. */
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size)
{
   struct inode *ino = get_inode(inum);
   ino->atime = time(NULL);
   put_inode(inum, ino);
   if(ino == NULL){
        printf("\tim: the file is not exist!\n");
        exit(0);
   }
   *size = ino->size;
   int tmp_size = *size;                
   char *_buf_out = (char*)malloc(ino->size);
   char *_buf_out_head = _buf_out;
   for(uint32_t i=0; i<NDIRECT; i++){
        if(tmp_size <= 0){
            *buf_out = _buf_out_head;
            return;
        }
        char tmp_buf[BLOCK_SIZE];
        bm->read_block(ino->blocks[i], tmp_buf);
        if(tmp_size >= BLOCK_SIZE)
            memcpy(_buf_out, tmp_buf, BLOCK_SIZE);
        else memcpy(_buf_out, tmp_buf, tmp_size % BLOCK_SIZE);
        _buf_out += BLOCK_SIZE;
        tmp_size -= BLOCK_SIZE;
   }
   if(tmp_size <= 0){
        *buf_out = _buf_out_head;
        return;
   }
   char tmp_buf[BLOCK_SIZE];
   bm->read_block(ino->blocks[NDIRECT], tmp_buf);
   for(uint32_t i=0; i<BLOCK_SIZE; i++){
        if(tmp_size <= 0){
            *buf_out = _buf_out_head;
            return;
        }
        uint *p = (uint*)tmp_buf + i;
        char buf[BLOCK_SIZE];
        bm->read_block(*p, buf);
        if(tmp_size >= BLOCK_SIZE)
            memcpy(_buf_out, buf, BLOCK_SIZE);
        else memcpy(_buf_out, buf, tmp_size % BLOCK_SIZE);
        _buf_out += BLOCK_SIZE;
        tmp_size -= BLOCK_SIZE;
   }
   *buf_out = _buf_out_head;
   return;
}

void
inode_manager::write_file(uint32_t inum, const char *buf, int size)
{
   struct inode *ino = get_inode(inum);
   if(ino == NULL){
        printf("\tim: the file is not exist!\n");
        exit(0);
   }
   if(size > MAXFILE * BLOCK_SIZE){
        printf("\tim: the file size is too large!\n");
        exit(0);
   }

   int tmp_size = size;               
   for(uint32_t i=0; i<NDIRECT; i++){
        if(tmp_size <= 0){
            if(ino->blocks[i] != 0){
                bm->free_block(ino->blocks[i]);
                ino->blocks[i] = 0;
                continue;
            }else {
                break;
            }
        }
        if(ino->blocks[i] == 0){
            ino->blocks[i] = bm->alloc_block();
        }
        char tmp[BLOCK_SIZE];
        memset(tmp,0,BLOCK_SIZE);
        memcpy(tmp,buf,MIN(tmp_size,BLOCK_SIZE));
        bm->write_block(ino->blocks[i], tmp);
        tmp_size -= BLOCK_SIZE;
        buf += BLOCK_SIZE;
   }
   bool is_write_over = (tmp_size <= 0) ? 1 : 0;    
   if(tmp_size <= 0 && ino->blocks[NDIRECT] == 0){
   }else {
        if(ino->blocks[NDIRECT] == 0){
            ino->blocks[NDIRECT] = bm->alloc_block();
        }
        char tmp_buf[BLOCK_SIZE];
        bm->read_block(ino->blocks[NDIRECT], tmp_buf);
        for(uint32_t i=0; i<BLOCK_SIZE; i++){
            uint *p = (uint*)tmp_buf + i;
            if(tmp_size <= 0){
                if(*p != 0){
                    bm->free_block(*p);
                    *p = 0;
                    continue;
                }else {
                    break;
                }
            }
            if(*p == 0){
                *p = bm->alloc_block();
            }
            char tmp[BLOCK_SIZE];
            memset(tmp,0,BLOCK_SIZE);
            memcpy(tmp,buf,MIN(tmp_size,BLOCK_SIZE));
            bm->write_block(*p, tmp);
            tmp_size -= BLOCK_SIZE;
            buf += BLOCK_SIZE;
        }
        if(is_write_over && ino->blocks[NDIRECT] != 0){
            bm->free_block(ino->blocks[NDIRECT]);
            ino->blocks[NDIRECT] = 0;
        }
        if(ino->blocks[NDIRECT] != 0)bm->write_block(ino->blocks[NDIRECT], tmp_buf);
   }
   ino->size = size;
   ino->mtime = time(NULL);
   put_inode(inum, ino);
  return;
}

void
inode_manager::getattr(uint32_t inum, extent_protocol::attr &a)
{
   struct inode *ino_disk;
   char buf[BLOCK_SIZE];
   bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
   ino_disk = (struct inode*)buf + inum%IPB;
   a.type = ino_disk->type;
   a.atime = ino_disk->atime;
   a.ctime = ino_disk->ctime;
   a.mtime = ino_disk->mtime;
   a.size = ino_disk->size;
   return;
}

void
inode_manager::setattr(uint32_t inum, extent_protocol::attr a)
{
   struct inode *ino_disk;
   char buf[BLOCK_SIZE];
   bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
   ino_disk = (struct inode*)buf + inum%IPB;
   ino_disk->type = a.type;
   ino_disk->atime = a.atime;
   ino_disk->ctime = a.ctime;
   ino_disk->mtime = a.mtime;
   return;
}

void
inode_manager::remove_file(uint32_t inum)
{
    struct inode *ino = get_inode(inum);
    if(ino == NULL){
        printf("\tim: the file is not exist!\n");
        exit(0);
    }
    for(uint32_t i=0; i<NDIRECT; i++){
        if(ino->blocks[i] == 0)break;
        bm->free_block(ino->blocks[i]);
    }
    if(ino->blocks[NDIRECT] != 0){
        char tmp_buf[BLOCK_SIZE];
        bm->read_block(ino->blocks[NDIRECT], tmp_buf);
        for(uint32_t i=0; i<BLOCK_SIZE; i++){
            uint *p = (uint*)tmp_buf + i;
            if(*p == 0)break;
            bm->free_block(*p);
        }
        bm->free_block(ino->blocks[NDIRECT]);
    }
    free_inode(inum);
  return;
}
