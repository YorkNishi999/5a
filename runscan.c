#include <stdio.h>
#include "ext2_fs.h"
#include "read_ext2.h"
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

char buffer [1024];
char single_buffer [1024];
char double_buffer [1024];
char triple_buffer [1024];
int file_inode [1024];
int file_index = 0;

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

	//part1
	for (unsigned int i = 0; i < inodes_per_block * itable_blocks; i++) 
  {
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

    if (S_ISREG(inode->i_mode)) {
      lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);    /* position data block */
      memset(buffer, 0, block_size);
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
      // printf("is_jpg: %d, inode num is [%d]\n", is_jpg, i );

      char* inode_num = malloc(sizeof(i));
      sprintf(inode_num, "%d", i);

    // only test inode13, need to be modified.
      if (is_jpg) {
        for(int j = 0; j < EXT2_N_BLOCKS; j++) {

          if(inode->i_block[j] == 0) continue;

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

          FILE *fp = fopen(path, "a+");
          if (j < EXT2_IND_BLOCK) { // use const.
            lseek(fd, BLOCK_OFFSET(inode->i_block[j]), SEEK_SET);    /* position data block */
            memset(buffer, 0, block_size);
            int rv = read(fd, buffer, block_size);  // to be changed
            printf("rv in direct: %d\n", rv);

            // write filename to file

            if(fp == NULL) {
              printf("Fail to open file.");
              exit(0);
            } else {
              fwrite(buffer, 1, rv, fp); // not write full size block// to be changed
            }
						file_inode[file_index] = i;
            file_index++;
          }  // end of direct
          // fclose(fp);

          if (j == EXT2_IND_BLOCK) {
            lseek(fd, BLOCK_OFFSET(inode->i_block[j]), SEEK_SET);
            memset(single_buffer, 0, block_size);
            read(fd, single_buffer, block_size);

            int* single_block_num = (int *)single_buffer;
            // FILE *fp = fopen(path, "a+");
            for (int fi = 0; fi<256; fi++) {
              lseek(fd, BLOCK_OFFSET(single_block_num[fi]), SEEK_SET);
              memset(buffer, 0, block_size);
              int rv = read(fd, buffer, block_size);

              // write filename to file
              if(fp == NULL) {
                printf("Fail to open file.");
                exit(0);
              } else {
                fwrite(buffer, 1, rv, fp);
              }
            }
            // fclose(fp);
          }  // end of single

          if (j == EXT2_DIND_BLOCK) {
            lseek(fd, BLOCK_OFFSET(inode->i_block[j]), SEEK_SET);
            memset(double_buffer, 0, block_size);
            read(fd, double_buffer, block_size);
            int* double_block_num = (int *)double_buffer;

            for (int fi = 0; fi<256; fi++) {
              lseek(fd, BLOCK_OFFSET(double_block_num[fi]), SEEK_SET);
              memset(single_buffer, 0, block_size);
              read(fd, single_buffer, block_size);
              int* single_block_num = (int *)single_buffer;

              for (int fii = 0; fii<256; fii++) {
                lseek(fd, BLOCK_OFFSET(single_block_num[fii]), SEEK_SET);
                memset(buffer, 0, block_size);
                int rv = read(fd, buffer, block_size);
                // write filename to file
                // FILE *fp = fopen(path, "a+");
                if(fp == NULL) {
                  printf("Fail to open file.");
                  exit(0);
                } else {
                  fwrite(buffer, 1, rv, fp);
                }
                // fclose(fp);
              }
            }
          }  // end of double

          if (j == EXT2_TIND_BLOCK) {
            lseek(fd, BLOCK_OFFSET(inode->i_block[j]), SEEK_SET);
            memset(triple_buffer, 0, block_size);
            read(fd, triple_buffer, block_size);
            int* triple_block_num = (int *)triple_buffer;

            for (int fi = 0; fi<256; fi++) {
              lseek(fd, BLOCK_OFFSET(triple_block_num[fi]), SEEK_SET);
              memset(double_buffer, 0, block_size);
              read(fd, double_buffer, block_size);
              int* double_block_num = (int *)double_buffer;

              for (int fii = 0; fii<256; fii++) {
                lseek(fd, BLOCK_OFFSET(double_block_num[fii]), SEEK_SET);
                memset(single_buffer, 0, block_size);
                read(fd, single_buffer, block_size);
                int* sinble_block_num = (int*)single_buffer;
                
                for(int fiii = 0; fiii<256; fiii++) {
                  lseek(fd, BLOCK_OFFSET(sinble_block_num[fii]), SEEK_SET);
                  memset(buffer, 0, block_size);
                  read(fd, buffer, block_size);
                  
                  // write filename to file
                  // FILE *fp = fopen(path, "a+");
                  if(fp == NULL) {
                    printf("Fail to open file.");
                    exit(0);
                  } else {
                    fwrite(buffer, 1, 1024, fp);
                  }
                  // fclose(fp);
                }
              }
            }
          }  // end of triple
          fclose(fp);
        } // end of for (j)

				// truncate the file
				char str1[] = "./";
				char str2[] = "/file-";
				char str3[] = ".jpg";
				int len = strlen(str1) + strlen(argv[2]) + strlen(str2) + strlen(inode_num) + strlen(str3) + 1;
				char path2[len];
				memset(path2, '\0', len);
				strcat(path2, str1);
				strcat(path2, argv[2]);
				strcat(path2, str2);
				strcat(path2, inode_num);
				strcat(path2, str3);

				int fd = open(path2 , O_RDWR);
        ftruncate( fd, inode->i_size);
				close(fd);

      }
    }
	}


  for (unsigned int i = 0; i < itable_blocks*inodes_per_block; i++) { //  part222222222222222222222
		struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
		read_inode(fd, 0, start_inode_table, i, inode);
		
		unsigned int i_blocks = inode->i_blocks/(2<<super.s_log_block_size);
		if (i_blocks == 0) continue;

		// Part2: record filenames -> can output the names, but can;'t get the filename which has been deleted.
		if (S_ISDIR(inode->i_mode)) 
		{
			struct ext2_dir_entry *dentry = malloc(sizeof(struct ext2_dir_entry));
			lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);
      memset (buffer, 0, block_size);
			read(fd, buffer, sizeof(struct ext2_dir_entry));

			int entry_offset = 0 ;

			while (entry_offset < (int)block_size) 
			{
				dentry = (struct ext2_dir_entry*) & (buffer[0 + entry_offset]);

        struct ext2_inode *inode_dentry = malloc(sizeof(struct ext2_inode));
				read_inode(fd, 0, start_inode_table, dentry->inode, inode_dentry);
        
				int name_len = dentry->name_len & 0xFF; // convert 2 bytes to 4 bytes properly
				char name [EXT2_NAME_LEN];
				strncpy(name, dentry->name, name_len);
				name[name_len] = '\0';

        // create file and rename
        if (S_ISREG(inode_dentry->i_mode)) {
			
					for (int i = 0; i < file_index; i++){
						if ((int)dentry->inode == file_inode[i]){
							char buffer[inode_dentry->i_size];

							char* inode_num = malloc(sizeof(i)); 
							sprintf(inode_num, "%d", dentry->inode);

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
							FILE *fp = fopen(path, "r");
							if(fp == NULL) {
								printf("Fail to open file.");
								exit(0);
							}
							else {
								fread(buffer, inode_dentry->i_size, 1, fp);
								char str4[] = "/";
								int len = strlen(str1) + strlen(argv[2]) + strlen(str4) + dentry->name_len + 1;
								char path[len];
								memset(path, '\0', len);
								strcat(path, str1);
								strcat(path, argv[2]);
								strcat(path, str4);
								strcat(path, name);
								FILE *fp2 = fopen(path, "w+");
								printf("path : %s\n", path);
								fwrite(buffer, inode_dentry->i_size, 1, fp2);
								fclose(fp2);
							}
							fclose(fp);
						}
					}
					printf("Entry name is --%s--; Inode num is [%d]\n", name, dentry->inode);
				}

        // find the hidden file
				int name_len_new;
				if ((name_len % 4) != 0)
					name_len_new = name_len + (4 - (name_len % 4));
				else
					name_len_new = name_len;


				if(dentry->rec_len != (name_len_new + 8)) 
					entry_offset = entry_offset + name_len_new + 8;
				else 
				  entry_offset = entry_offset + dentry->rec_len;	
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
