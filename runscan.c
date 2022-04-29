#include <stdio.h>
#include "ext2_fs.h"
#include "read_ext2.h"

void traverse_inode(off_t offset, struct ext2_inode* next_inode) {

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
	printf("1st fd: %d\n", fd);
	read_group_desc(fd, 0, &group);
	printf("2nd fd: %d\n", fd);

	printf("koko\n");
	printf("There are %u inodes in an inode table block and %u blocks in the inode table\n", inodes_per_block, itable_blocks);
	//iterate the first inode block
	off_t start_inode_table = locate_inode_table(0, &group); // offsetをinode_tableのとこに持ってく関数
	
	for (unsigned int i = 0; i < inodes_per_block; i++) { // inodes_per_block = 8 (1ブロックにinode:128byte いくつあるか？)
		printf("inode %u: \n", i);
		struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));

		read_inode(fd, 0, start_inode_table, i, inode);
		/* the maximum index of the i_block array should be computed from i_blocks / ((1024<<s_log_block_size)/512)* or once simplified, i_blocks/(2<<s_log_block_size)* https://www.nongnu.org/ext2-doc/ext2.html#i-blocks*/

		unsigned int i_blocks = inode->i_blocks/(2<<super.s_log_block_size);
		printf("number of blocks: %u, inode->i_blocks: %u\n", i_blocks, inode->i_blocks);
		printf("Is directory? %s \n Is Regular file? %s\n", S_ISDIR(inode->i_mode) ? "true" : "false",
					S_ISREG(inode->i_mode) ? "true" : "false");
		printf("size: %u\n", inode->i_size);
		
		// print i_block numberss
		for(unsigned int i=0; i<EXT2_N_BLOCKS; i++) { // EXT2_N_BLOCKS = 15
			// printf("current file offset (byte): %lu\n",lseek(fd, 0, SEEK_CUR));
			// printf("current file offset (block): %lu\n",lseek(fd, 0, SEEK_CUR)/BASE_OFFSET);
			if (i < EXT2_NDIR_BLOCKS){                                 /* direct blocks */
				printf("Block %2u : %u\n", i, inode->i_block[i]);
			} else if (i == EXT2_IND_BLOCK) {                             /* single indirect block */
				printf("Single   : %u\n", inode->i_block[i]);
			} else if (i == EXT2_DIND_BLOCK) {                            /* double indirect block */
				printf("Double   : %u\n", inode->i_block[i]);
			} else if (i == EXT2_TIND_BLOCK) {                            /* triple indirect block */
				printf("Triple   : %u\n", inode->i_block[i]);
			}
			off_t tmp;
			if (S_ISDIR(inode->i_mode) && ((tmp = inode->i_block[i]) != 0) ) {
				printf("Directory inode\n");

			} else if (S_ISREG(inode->i_mode) && ((tmp = inode->i_block[i]) != 0)) {
				lseek(fd, BLOCK_OFFSET(tmp), SEEK_SET); // a: 217, tmp: 217

				for(unsigned int j = 0; j<inodes_per_block; j++) {
					printf("\tnext_inode %u: \n", j);
					struct ext2_inode* next_inode = malloc(sizeof(struct ext2_inode));
					read_inode(fd, 0, BLOCK_OFFSET(tmp), j, next_inode); 

					unsigned int new_i_blocks = next_inode->i_blocks/(2<<super.s_log_block_size);
					// print inode info
					printf("\t\tsize: %u\n",next_inode->i_size);
					printf("\t\tnumber of blocks: %u, inode->i_blocks: %u\n", new_i_blocks, next_inode->i_blocks);
					printf("\t\tIs directory? %s, Is Regular file? %s\n", S_ISDIR(next_inode->i_mode) ? "true" : "false", 
										S_ISREG(next_inode->i_mode) ? "true" : "false");
					// print inode block info
					for(unsigned int j=0; j<EXT2_N_BLOCKS; j++) {
						if (j < EXT2_NDIR_BLOCKS){                                 /* direct blocks */
							printf("\t\tBlock %2u : %u\n", j, next_inode->i_block[j]);
						} else if (j == EXT2_IND_BLOCK) {                             /* single indirect block */
							printf("\t\tSingle   : %u\n", next_inode->i_block[j]);
						} else if (j == EXT2_DIND_BLOCK) {                            /* double indirect block */
							printf("\t\tDouble   : %u\n", next_inode->i_block[j]);
						} else if (j == EXT2_TIND_BLOCK) {                            /* triple indirect block */
							printf("\t\tTriple   : %u\n", next_inode->i_block[j]);
						}
					}
				}
			}
		}
		
		free(inode);

	}

	
	close(fd);
}
