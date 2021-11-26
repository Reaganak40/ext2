/************* read_cat.c file **************/

extern int get_block(int dev, int blk, char *buf);
extern int get_indirect_block(int dev, int idblk, int blk_number);
extern int get_double_indirect_block(int dev, int didblk, int blk_number);


int my_read(int fd, char* buf, int nbytes){

    OFT* table;

    int count = 0; // number of bytes read
    int avil, offset, lbk, start, dblock, remain; // bytes available, oft offset,
                                                  // logical block, start point within data block, 
                                                  // actual data block, remaining bytes in datablock
    char b_buf[BLKSIZE]; // block buf
    char* cp; // char pointer for b_buf

    table = oget(running, fd); //get open file table for file descriptor

    if(nbytes < 0){
        printf("my_read : very sneaky to make nbytes negative.\n");
        printf("my_read unsuccessful\n");
        return -1;
        
    }

    if(!table){ // there is no fd at this location in proc fd list
        printf("my_read unsuccessful\n");

        return -1;

    }

    if(table->mode == W){
        printf("my_read : fd is not in read mode\n");
        printf("my_read unsuccessful\n");
        return -1;
        
    }

    offset = table->offset;
    avil = (table->minodePtr->INODE.i_size) - offset;

    if(avil < 0){ //if avil is negative (offset is way too big)
        printf("my_read : offset exceeded file size\n");
        printf("my_read unsuccessful\n");
        return -1;
    }

    while(nbytes && avil){ // while there are more bytes requested to be read, and there are bytes avialable 
        lbk = offset / BLKSIZE; //what i_block would the data be at
        start = offset % BLKSIZE; // remainder is the byte offset at that block

        if(lbk > 12 && lbk <= (BLKSIZE / 4) + 12){ // exceeds direct i_block limit : INDIRECT BLOCK
            int indirect_block;
            indirect_block = table->minodePtr->INODE.i_block[12];

            if(indirect_block == 0){ // if no indrect block
                printf("my_read : could not find indrect disk block\n");
                printf("my_read unsuccessful\n");
                return -1;
            }

            lbk -= 12; //since this is after the first 12 i_blocks, shift blk value

            dblock = get_indirect_block(dev, indirect_block, lbk); // get indirect block number

            if(dblock == -1 || dblock == 0){
                printf("my_read unsuccessful\n");
                return -1;
            }

        }else if(lbk > (BLKSIZE / 4) + 12){ // DOUBLE INDIRECT BLOCK
            
            int double_indirect_block;
            double_indirect_block = table->minodePtr->INODE.i_block[13];

            if(double_indirect_block == 0){ // if no double indrect block
                printf("my_read : could not find dobule indrect disk block\n");
                printf("my_read unsuccessful\n");
                return -1;
            }

            lbk -= (BLKSIZE / 4) + 12; //since this is after the first 12 i_blocks and indirect blocks, shift blk value

            dblock = get_double_indirect_block(dev, double_indirect_block, lbk); // get indirect block number

            if(dblock == -1 || dblock == 0){
                printf("my_read unsuccessful\n");
                return -1;
            }
            
        }else{

            dblock = table->minodePtr->INODE.i_block[lbk];
        }

        if(dblock == 0){ // if i_blocks don't stretch that far
            printf("my_read : the requested i_block is unassigned\n");
            printf("my_read unsuccessful\n");
            return -1;
        }

        get_block(dev, dblock, b_buf); //get the data block

        cp = b_buf + start;
        remain = BLKSIZE - start;

        // REVISED ALGORTHM - much faster reads in chunks rather than indivual bytes
        int chunk; // number of bytes read in this chunk

        if(remain > avil){ // chunk will either extend to end of block or the end of data (given by avil)
            chunk = avil;
        }else{
            chunk = remain;
        }
        
        if(chunk > nbytes) { // if chunk is more than nbytes desired to be read
            chunk = nbytes;
        }

        memcpy(buf, cp, chunk);
        buf += chunk; cp += chunk;

        offset+=chunk; count+= chunk; // offset increased by chunk size (CS), number of bytes read increased by CS
        remain-=chunk; avil-=chunk; nbytes-=chunk; // CS less remaining byte in data block, CS less bytes avialalbe in entire file, CS less byte needed to be read
        
        if(nbytes == 0 || avil == 0){ // if requested bytes read is reached or read last byte of file
            break;
        }
       
    } // while loop ends when all byte read limit reached or end of file

    table->offset = offset; // apply new offset to fd
    return count;
}

int my_cat(char* pathname){

    int fd;
    int size;
    char buf[BLKSIZE + 1];
    int bytes_read, local_read;

    if(!getino(pathname)){ //if file does not exist
        printf("my_cat : %s does not exist\n", pathname);
        printf("my_cat unsuccessful\n");
        return -1;
    }

    fd = my_open(pathname, R);

    if(fd == -1){
        printf("my_cat unsuccessful\n");
        return -1;
    }

    size = oget(running, fd)->minodePtr->INODE.i_size;
    bytes_read = 0;

    printf("[cat] start:\n\n"); 
    while(bytes_read < size){

        local_read = my_read(fd, buf, BLKSIZE); // read as many bytes as possible in a data block

        if(local_read == -1){
            printf("my_cat : interrupt, failed to read all\n");
            return -1;
        }
        bytes_read += local_read;

        buf[local_read] = '\0';
        printf("%s", buf); // print file content to the screen
    }

    my_close(fd);

    //printf("\nbytes read: %d\n", bytes_read);

    return 0;
}