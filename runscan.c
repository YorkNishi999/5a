#include <stdio.h>
#include "ext2_fs.h"
#include "read_ext2.h"
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

char buffer [1024];

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("expected usage: ./runscan inputfile outputfile\n");
		exit(0);
	}

	char *dir = argv[2];
	//check if uotput directory has already existed
	if(opendir(argv[2]) == NULL) {
    mkdir(dir, S_IRWXU);
	}
	else {
		printf("output directory already exists.\n");
		exit(0);
	}
	
	int fd;

	fd = open(argv[1], O_RDONLY);    /* open disk image */

	ext2_read_init(fd);

	struct ext2_super_block super;
	struct ext2_group_desc group;
	
	// example read first the super-block and group-descriptor
	read_super_block(fd, 0, &super);
	read_group_desc(fd, 0, &group);
	
	printf("There are %u inodes in an inode table block and %u blocks in the idnode table\n", inodes_per_block, itable_blocks);
	//iterate the first inode block
	off_t start_inode_table = locate_inode_table(0, &group);
	for (unsigned int i = 0; i < inodes_per_block * itable_blocks; i++) {
		struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
		read_inode(fd, 0, start_inode_table, i, inode);
		if (inode->i_blocks == 0) continue;
		printf("inode %u: \n", i);
		/* the maximum index of the i_block array should be computed from i_blocks / ((1024<<s_log_block_size)/512)
			* or once simplified, i_blocks/(2<<s_log_block_size)
			* https://www.nongnu.org/ext2-doc/ext2.html#i-blocks
			*/
		unsigned int i_blocks = inode->i_blocks/(2<<super.s_log_block_size);
		printf("number of blocks %u\n", i_blocks);
		printf("Is directory? %s \n Is Regular file? %s\n",
			S_ISDIR(inode->i_mode) ? "true" : "false",
			S_ISREG(inode->i_mode) ? "true" : "false");

		// Part1: scan all jpg files  -> can extract jpg file, but only for the pic only use direct block. naming problem wasn't solved. 
		if (S_ISREG(inode->i_mode)) 
		{

		lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);    /* position data block */
		memset (buffer, 0, block_size);
		read(fd, buffer, block_size);    				/* read data block */

		int is_jpg = 0;
		if (buffer[0] == (char)0xff &&
				buffer[1] == (char)0xd8 &&
				buffer[2] == (char)0xff &&
				(buffer[3] == (char)0xe0 ||
				buffer[3] == (char)0xe1 ||
				buffer[3] == (char)0xe8)) {
			is_jpg = 1;
		}
		printf("is_jpg: %d, inode num is [%d]\n", is_jpg, i );
			
		char* inode_num = malloc(sizeof(i)); 
		sprintf(inode_num, "%d", i);

		// only test inode13, need to be modified.
		if (is_jpg) 
		{ 
			for(int j = 0; j < EXT2_N_BLOCKS; j++) 
			{

				if(inode->i_block[j] == 0) continue;

				lseek(fd, BLOCK_OFFSET(inode->i_block[j]), SEEK_SET);    /* position data block */
				memset (buffer, 0, block_size);
				read(fd, buffer, block_size);    	
				//printf("r_value: %d\n", r_value);
				//printf("J:   %d\n", j);
				//printf("Blokc num:   %d\n", inode->i_block[j]);


				// filename path
				char str1[] = "./";
				char str2[] = "/file-";
				char str3[] = ".jpg";
				int len = strlen(str1) + strlen(argv[2]) + strlen(str2) + strlen(inode_num) + strlen(str3) + 1;
				char path[len];
				memset(path, '\0', len);
				strcat(path, str1);
				strcat(path, argv[2]);
				strcat(path, str2);
				strcat(path, inode_num);
				strcat(path, str3);
				// write filename to file
				FILE *fp = fopen(path, "a+"); 
				if(fp == NULL) {
					printf("Fail to open file.");
					exit(0);
				}
				else {
					fwrite(buffer, 1, 1024, fp);
				}
				fclose(fp);
			}
		}
	}

		// Part2: record filenames -> can output the names, but can;'t get the filename which has been deleted.
		if (S_ISDIR(inode->i_mode)) 
		{
			struct ext2_dir_entry *dentry = malloc(sizeof(struct ext2_dir_entry));
			lseek(fd, (int)inode->i_block[0] * block_size, SEEK_SET); // position data block */
			read(fd, buffer, sizeof(struct ext2_dir_entry));

			int entry_offset = 0 ;

			while (entry_offset != (int)block_size) 
			{
				dentry = (struct ext2_dir_entry*) & (buffer[0 + entry_offset]);

				int name_len = dentry->name_len & 0xFF; // convert 2 bytes to 4 bytes properly
				char name [EXT2_NAME_LEN];
				strncpy(name, dentry->name, name_len);
				name[name_len] = '\0';
				entry_offset = entry_offset + dentry->rec_len;
				struct ext2_inode *inode_check = malloc(sizeof(struct ext2_inode));
				read_inode(fd, 0, start_inode_table, dentry->inode, inode_check);
				
				if (S_ISREG(inode_check->i_mode)) 
				{
					// filename path
					char str1[] = "./";
					char str2[] = "/filename.txt";
					int len = strlen(str1) + strlen(argv[2]) + strlen(str2) + 1;
					char path[len];
					memset(path, '\0', len);
					strcat(path, str1);
					strcat(path, argv[2]);
					strcat(path, str2);
					// write filename to file
					FILE *fp = fopen(path, "a+"); 
					if(fp == NULL) {
						printf("Fail to open file.");
						exit(0);
					}
					else {
						fputs(name, fp);
						fputs("\n", fp);
					}
					fclose(fp);
					
					printf("Entry name is --%s--; Inode num is [%d]\n", name, dentry->inode);
				}
			}
		}
		
		// print i_block numberss
		for(unsigned int i=0; i<EXT2_N_BLOCKS; i++)
		{       if (i < EXT2_NDIR_BLOCKS)                                 /* direct blocks */
						printf("Block %2u : %u\n", i, inode->i_block[i]);
				else if (i == EXT2_IND_BLOCK)                             /* single indirect block */
						printf("Single   : %u\n", inode->i_block[i]);
				else if (i == EXT2_DIND_BLOCK)                            /* double indirect block */
						printf("Double   : %u\n", inode->i_block[i]);
				else if (i == EXT2_TIND_BLOCK)                            /* triple indirect block */
						printf("Triple   : %u\n", inode->i_block[i]);
		}
		free(inode);
	}		
	close(fd);
}
