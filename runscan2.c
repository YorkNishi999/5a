#include <stdio.h>
#include <string.h>
#include "ext2_fs.h"
#include "read_ext2.h"

int level_count = -1;
char buffer [1024];

int is_jpgs(char buffer[1024]) {
  if (buffer[0] == (char)0xff && buffer[1] == (char)0xd8 && buffer[2] == (char)0xff && 
        (buffer[3] == (char)0xe0 || buffer[3] == (char)0xe1 || buffer[3] == (char)0xe8)) {
    return 1;
  }
  return 0;
}

void traverse_inode(int fd, off_t offset, __u32 s_log_block_size, int img) {
  level_count++; // for debug

  for (unsigned int inode_idx = 0; inode_idx < inodes_per_block * itable_blocks; inode_idx++) { // inodes_per_block = 8 (1ブロックにinode:128byte いくつあるか？)
    struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
    read_inode(fd, 0, offset, inode_idx, inode);
    if(inode->i_size == 0) {
      continue;
    }
    printf("img: %d, level: %d, inode %u: \n", img, level_count, inode_idx);


    // information on inode
    // unsigned int i_blocks = inode->i_blocks/((1024<<s_log_block_size)/512);
    unsigned int i_blocks = inode->i_blocks/(2<<s_log_block_size);
    printf("number of blocks: %u, inode->i_blocks: %u\n", i_blocks, inode->i_blocks);
    if (!S_ISREG(inode->i_mode) && !S_ISDIR(inode->i_mode)) {
      continue;
    }
    printf("Is directory? %s. Is Regular file? %s\n", S_ISDIR(inode->i_mode) ? "true" : "false",
          S_ISREG(inode->i_mode) ? "true" : "false");
    printf("size: %u\n", inode->i_size);

    // Part1: scan all jpg files  -> can extract jpg file, 
    // but only for the pic only use direct block. naming problem wasn't solved. 
    if (S_ISREG(inode->i_mode)) {
      lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET); /* position data block */
      memset(buffer, 0, block_size);
      read(fd, buffer, block_size); /* read data block */

      int is_jpg = is_jpgs(buffer);
      printf("\tis_jpg: %d, inode num is [%d]\n", is_jpg, inode_idx); // for debug

      // for img file name
      char* inode_num = malloc(sizeof(inode_idx)); 
      sprintf(inode_num, "%d", inode_idx);

      if (is_jpg) {
        for(unsigned int i=0; i<EXT2_N_BLOCKS; i++) { // EXT2_N_BLOCKS = 15
          // printf("current file offset (byte): %lu\n",lseek(fd, 0, SEEK_CUR));
          // printf("current file offset (block): %lu\n",lseek(fd, 0, SEEK_CUR)/BASE_OFFSET);
          if (inode->i_block[i] == 0) continue;

          // off_t a = lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET); /* position data block */
          lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET); /* position data block */
          memset(buffer, 0, block_size);
          read(fd, buffer, block_size); /* read data block */

          // filename path
          char str1[] = "./";
          char str2[] = "/file-";
          char str3[] = ".jpg";
          char dir[] = "test";
          int len = strlen(str1) + strlen(dir) + strlen(str2) + strlen(inode_num) + strlen(str3) + 1;
          char path[len];
          memset(path, '\0', len);
          strcat(path, str1);
          strcat(path, dir);
          strcat(path, str2);
          strcat(path, inode_num);
          strcat(path, str3);

          // write filename to file
          FILE *fp = fopen(path, "a+"); 
          if (fp == NULL) {
            printf("Fail to open file.");
            exit(0);
          } else {
            fwrite(buffer, 1, 1024, fp);
          }
          fclose(fp);

          // print i_block numberss
          if (i < EXT2_NDIR_BLOCKS){                                 /* direct blocks */
            printf("\timg: %d, level: %d, inode: %u, Block %2u: %u\n", img, level_count, inode_idx, i, inode->i_block[i]);
          } else if (i == EXT2_IND_BLOCK) {                             /* single indirect block */
            printf("\timg: %d, level: %d, inode: %u, Single : %u\n", img, level_count, inode_idx, inode->i_block[i]);
          } else if (i == EXT2_DIND_BLOCK) {                            /* double indirect block */
            printf("\timg: %d, level: %d, inode: %u, Double : %u\n", img, level_count, inode_idx, inode->i_block[i]);
          } else if (i == EXT2_TIND_BLOCK) {                            /* triple indirect block */
            printf("\timg: %d, level: %d, inode: %u, Triple : %u\n", img, level_count, inode_idx, inode->i_block[i]);
          }

          // traverse_inode(fd, a, s_log_block_size, 1);
        }
      } // end of if (is_jpg)
      
    } // end of if (S_ISREG(inode->i_mode)), part1

    free(inode);
  }
  level_count--;
} // end of traverse function

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

  traverse_inode(fd, start_inode_table, super.s_log_block_size, 1);
  // the last parameter = 1 when image, 0 when file name

  close(fd);
}