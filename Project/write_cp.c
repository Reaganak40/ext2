/************* write_cp.c file **************/

extern int get_block(int dev, int blk, char *buf);


int my_write(int fd, char* buf, int nbytes){
    
    int count = 0, offset;
    OFT* table;

    int lbk, start, buf_loc = 0, remain, fileSize; // logical block, start for write, where we are look at buf, remaining room in current data block, file size
    char bbuf[BLKSIZE], *cp;
    int d_block; // the actual data block number
    int nblk; // if we need a new datas block

    table = oget(running, fd); //get open file table for file descriptor

    if(nbytes < 0){
        printf("my_write : very sneaky to make nbytes negative.\n");
        printf("my_write unsuccessful\n");
        return -1;
        
    }

    if(!table){ // there is no fd at this location in proc fd list
        printf("my_write unsuccessful\n");

        return -1;

    }

    if(table->mode == R){
        printf("my_write : fd is not in write mode\n");
        printf("my_write unsuccessful\n");
        return -1;
        
    }

    if(table->mode == APPEND){
        offset = table->minodePtr->INODE.i_size; //if append begin writing at the end of the file
    }else{
        offset = table->offset; // if write or read-write, then write at fd offset
    }

    fileSize = table->minodePtr->INODE.i_size;
    while(nbytes){ //write to file until bytes limit reached

        lbk = offset / BLKSIZE; //what i_block would the data be at
        start = offset % BLKSIZE; // remainder is the byte offset at that block
        remain = BLKSIZE - start;

        d_block = table->minodePtr->INODE.i_block[lbk];

        if(d_block == 0){ // if i_blocks don't stretch that far, allocate new data block
            printf("my_write : allocating a new datablock for inode...\n");
            nblk = balloc(dev); // get the newly allocated block number
            table->minodePtr->INODE.i_block[lbk] = nblk; // add new data block to i_block list
            d_block = nblk;
        }

        get_block(dev, d_block, bbuf); // get data block
        cp = bbuf + start;

        int chunk;
        if(nbytes < remain){
            chunk = nbytes; // if bytes left to write does not expand entire data block -> chunk is nbytes
        }else{
            chunk = remain; // if the rest of data block can be written and still remaining nbytes -> chunk is remain
        }
        printf("writing data...\n");
        memcpy(cp, buf + buf_loc, chunk); //write a chunk of data to the data block
        
        nbytes -= chunk; 
        offset += chunk;
        count += chunk;
        buf_loc += chunk;

        put_block(dev, d_block, bbuf); // write block back to disk

        if(offset > fileSize){ //if file size has been increased
            fileSize = offset;
        }
    }

    table->offset = offset;
    if(table->minodePtr->INODE.i_size != fileSize){
        table->minodePtr->INODE.i_size = fileSize;
        table->minodePtr->dirty = 1; // dirty, will be modified on close
    }

    return count;
}