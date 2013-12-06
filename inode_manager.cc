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
  /*
   * your lab1 code goes here.
   * note: you should mark the corresponding bit in block bitmap when alloc.
   * you need to think about which block you can start to be allocated.
   */

  char bbuf[BLOCK_SIZE];
  for(blockid_t i = sb.nblocks/BPB + INODE_NUM/IPB + 2; i < BLOCK_NUM; i++)
  {
	  read_block(BBLOCK(i), bbuf);
	  int off = i % BPB;
	  int offbit = off % 8;
	  char *bitmap = (char *)bbuf + off/8;
	  int u = 1<<offbit;
	  if(((*bitmap)&u) == 0)
	  {
		  *bitmap = (*bitmap)|u;
		  write_block(BBLOCK(i),bbuf);
		  char none[BLOCK_SIZE];
		  memset(none, 0, sizeof(char) * BLOCK_SIZE);
		  write_block(i,none);
		  return i;
	  }
  }
  return 0;
}


void
block_manager::free_block(uint32_t id)
{
  /* 
   * your lab1 code goes here.
   * note: you should unmark the corresponding bit in the block bitmap when free.
   */
  char bbuf[BLOCK_SIZE];
  read_block(BBLOCK(id), bbuf);
  int off = id % BPB;
  int offbit = off % 8;
  char *bitmap = ( char *)bbuf + off/sizeof(char);
  int u = ~(1<<offbit);
  *bitmap = (*bitmap)&u;
  write_block(BBLOCK(id), bbuf);
  return;
}


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
  /* 
   * your lab1 code goes here.
   * note: the normal inode block should begin from the 2nd inode block.
   * the 1st is used for root_dir, see inode_manager::inode_manager().
   */
  uint32_t i;
  for(i=1;i<INODE_NUM;i++)
	if(inode_use[i] == 0)
		break;
  if(i == INODE_NUM)
  {
	printf("Disk is full!\n");
	return 0;
  }
  inode_use[i] = 1;
  struct inode *ino = (struct inode*)malloc(sizeof(struct inode));
  ino->type = type;
  put_inode(i,ino);
  return i;
}

void
inode_manager::free_inode(uint32_t inum)
{
  /* 
   * your lab1 code goes here.
   * note: you need to check if the inode is already a freed one;
   * if not, clear it, and remember to write back to disk.
   */
  struct inode * ino = get_inode(inum);
  if(ino->type != 0)
  {
	  ino->type = 0;
  	  put_inode(inum,ino);
  }
  free(ino);
  return;
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
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

#define MIN(a,b) ((a)<(b) ? (a) : (b))

/* Get all the data of a file by inum. 
 * Return alloced data, should be freed by caller. */
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size)
{
	/*
	* your lab1 code goes here.
	* note: read blocks related to inode number inum,
	* and copy them to buf_Out
	*/
	struct inode *ino = get_inode(inum);
	if(ino == NULL){
		printf("\tim: no file\n");
		free(ino);
		return;
	}
	*size = ino->size;
	int s = *size;
	char *buf = (char*)malloc(ino->size);
	char *buf_p = buf;
	for(int i=0; i<NDIRECT; i++){
		if(s <= 0){
			*buf_out = buf_p;
			free(ino);
			return;
		}
		char tbuf[BLOCK_SIZE];
		bm->read_block(ino->blocks[i], tbuf);
		if(s >= BLOCK_SIZE)
			memcpy(buf, tbuf, BLOCK_SIZE);
		else memcpy(buf, tbuf, s);
		buf += BLOCK_SIZE;
		s -= BLOCK_SIZE;
	}
	if(s>0)
	{
	   char nbuf[BLOCK_SIZE];
	   bm->read_block(ino->blocks[NDIRECT], nbuf);
	   for(int i=0; i<BLOCK_SIZE; i++){
			if(s <= 0){
				*buf_out = buf_p;
				free(ino);
				return;
			}
			blockid_t *p = (blockid_t*)nbuf + i;
			char tbuf[BLOCK_SIZE];
			bm->read_block(*p, tbuf);
			if(s >= BLOCK_SIZE)
				memcpy(buf, tbuf, BLOCK_SIZE);
			else memcpy(buf, tbuf, s);
			buf += BLOCK_SIZE;
			s -= BLOCK_SIZE;
	   }
	}
	*buf_out = buf_p;
	free(ino);
	return;
}



/* alloc/free blocks if needed */
void
inode_manager::write_file(uint32_t inum, const char *buf, int size)
{
  /*
   * your lab1 code goes here.
   * note: write buf to blocks of inode inum.
   * you need to consider the situation when the size of buf 
   * is larger or smaller than the size of original inode
   */

  if(size > MAXFILE*BLOCK_SIZE)
  {
	  printf("\tim: File is too large\n");
	  return;
  }
  blockid_t i;
  blockid_t k = 0;
  blockid_t indirect = 0;
  int m = 0;
  int bn = 0;
  char bbuf[BLOCK_SIZE];
  struct inode *ino = get_inode(inum);
  ino->size = size;
  char tbuf[BLOCK_SIZE];
  bool first = true;
  for(int in = 0; in<NDIRECT; in++)
  {
	  if(ino->blocks[in]!=0)
	  {
		  bm->free_block(ino->blocks[in]);
	  }
  }
  if(ino->blocks[NDIRECT]!=0)
  {
	  bm->read_block(ino->blocks[NDIRECT],tbuf);
	  for(int indirect = 0; indirect < NINDIRECT; indirect ++)
	  {
		  blockid_t *bid = (blockid_t *)tbuf + indirect;
		  if(*bid!=0)
			  bm->free_block(*bid);
	  }
	  bm->free_block(ino->blocks[NDIRECT]);
  }
  for(int j = size; j>0; j=j-BLOCK_SIZE)
  {
	  if(k<NDIRECT)
	  {
		  i = bm->alloc_block();
		  if(i>0)
		  {
			  ino->blocks[k++] = i;
		  }
		  else
		  {
			  printf("\tim: no block available\n");
			  free(ino);
			  return;
		  }
	  }
	  else
	  {
		  if(first)
		  {
			  indirect = bm->alloc_block();
			  if(!indirect)
			  {
				  printf("\tim: No block is available\n");
				  free(ino);
				  return;
			  }
			  ino->blocks[NDIRECT] = indirect;
			  bm->read_block(indirect,bbuf);
			  first = false;
		  }
		  blockid_t *b = (blockid_t*)bbuf + m;
		  i = bm->alloc_block();
		  *b = i;
		  bm->write_block(indirect,bbuf);
		  m++;
	  }
	  if(j<512)
		  memcpy(tbuf,buf + bn,j);
	  else
		  memcpy(tbuf,buf + bn,BLOCK_SIZE);
	  bm->write_block(i,tbuf);
	  bn += BLOCK_SIZE;


  }
  put_inode(inum,ino);
  free(ino);
  return;
}


void
inode_manager::getattr(uint32_t inum, extent_protocol::attr &a)
{
  /*
   * your lab1 code goes here.
   * note: get the attributes of inode inum.
   * you can refer to "struct attr" in extent_protocol.h
   */
  struct inode *ino;
  char buf[BLOCK_SIZE];
  if(inum<0 || inum>=INODE_NUM)
  {
	  printf("\tim: inum out of range\n");
	  return;
  }
  bm->read_block(IBLOCK(inum,bm->sb.nblocks),buf);
  ino = (struct inode*)buf + inum % IPB;
  if(ino->type == 0)
  {
	  printf("\tim: inode not exist\n");
	  return;
  }
  a.type = ino->type;
  return;
}

void
inode_manager::remove_file(uint32_t inum)
{
  /*
   * your lab1 code goes here
   * note: you need to consider about both the data block and inode of the file
   */
  struct inode * ino = get_inode(inum);
  char tbuf[BLOCK_SIZE];
  for(int in = 0; in<NDIRECT; in++)
  {
	  if(ino->blocks[in]!=0)
	  {
		  bm->free_block(ino->blocks[in]);
	  }
  }
  if(ino->blocks[NDIRECT]!=0)
  {
	  bm->read_block(ino->blocks[NDIRECT],tbuf);
	  for(int indirect = 0; indirect < NINDIRECT; indirect ++)
	  {
		  blockid_t *bid = (blockid_t *)tbuf + indirect;
		  if(*bid!=0)
			  bm->free_block(*bid);
		  else
			  break;
	  }
	  bm->free_block(ino->blocks[NDIRECT]);
  }
  free_inode(inum);
  free(ino);
  return;
}
