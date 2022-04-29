#include <stdio.h>
#include "ext2_fs.h"
#include "read_ext2.h"

int level_count = -1;

void traverse_inode(int fd, off_t offset, __u32 s_log_block_size) {
  level_count++;
	for (unsigned int i = 0; i < inodes_per_block; i++) { // inodes_per_block = 8 (1ブロックにinode:128byte いくつあるか？)
		printf("level: %d, inode %u: \n", level_count, i);
    struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
		read_inode(fd, 0, offset, i, inode);

    // information on inode
    unsigned int i_blocks = inode->i_blocks/((1024<<s_log_block_size)/512);
		printf("number of blocks: %u, inode->i_blocks: %u\n", i_blocks, inode->i_blocks);
		printf("Is directory? %s \n Is Regular file? %s\n", S_ISDIR(inode->i_mode) ? "true" : "false",
					S_ISREG(inode->i_mode) ? "true" : "false");
		printf("size: %u\n", inode->i_size);

		// print i_block numberss
		for(unsigned int i=0; i<EXT2_N_BLOCKS; i++) { // EXT2_N_BLOCKS = 15
			// printf("current file offset (byte): %lu\n",lseek(fd, 0, SEEK_CUR));
			// printf("current file offset (block): %lu\n",lseek(fd, 0, SEEK_CUR)/BASE_OFFSET);
      if(inode->i_block[i] == 0) {
        continue;
      }
			if (i < EXT2_NDIR_BLOCKS){                                 /* direct blocks */
				printf("Block %2u : %u\n", i, inode->i_block[i]);
			} else if (i == EXT2_IND_BLOCK) {                             /* single indirect block */
				printf("Single   : %u\n", inode->i_block[i]);
			} else if (i == EXT2_DIND_BLOCK) {                            /* double indirect block */
				printf("Double   : %u\n", inode->i_block[i]);
			} else if (i == EXT2_TIND_BLOCK) {                            /* triple indirect block */
				printf("Triple   : %u\n", inode->i_block[i]);
			}
      if (S_ISDIR(inode->i_mode) && (inode->i_block[i] != 0) ) {
				printf("Directory inode\n");
        // do something
			} else if (S_ISREG(inode->i_mode) && inode->i_block[i] != 0) {
				off_t a = lseek(fd, BLOCK_OFFSET(inode->i_block[i]), SEEK_SET); // a: 217, tmp: 217
        traverse_inode(fd, a, s_log_block_size);
      }
    }
    free(inode);
  }
}

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("expected usage: ./runscan inputfile outputfile\n");
		exit(0);
	}
	
	int fd;

	fd = open(argv[1], O_RDONLY);    /* open disk image */

	ext2_read_init(fd);
	printf("debug: num_groups: %d\n", num_groups); // for debug

	struct ext2_super_block super;
	struct ext2_group_desc group;
	
	// example read first the super-block and group-descriptor
	// この2つは変更する必要なし
	read_super_block(fd, 0, &super);
	read_group_desc(fd, 0, &group);

  // first 
	off_t start_inode_table = locate_inode_table(0, &group); // offsetをinode_tableのとこに持ってく関数

  traverse_inode(fd, start_inode_table, super.s_log_block_size);

  close(fd);
}