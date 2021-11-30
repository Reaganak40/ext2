/************* write_cp.c file **************/

// Function Declarations ************************
int my_cp(char* src, char* dest);
// **********************************************

extern int get_block(int dev, int blk, char *buf);
extern int make_indirect_block(int dev);
extern int add_indirect_entry(int dev, int idblk, int nblk);



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

    // WRITE THE BYTES TO THE BUF
    while(nbytes){ //write to file until bytes limit reached

        lbk = offset / BLKSIZE; //what i_block would the data be at
        int saved_lbk = lbk;
        start = offset % BLKSIZE; // remainder is the byte offset at that block
        remain = BLKSIZE - start;

        d_block = 0; // reset d_block for debugging purposes
        if(lbk >= 12 && lbk < (BLKSIZE / 4) + 12){ // exceeds direct i_block limit : INDIRECT BLOCK
            // NOTE: For floppy disk lbk would be between 12 (inclusive) and 268 (exclusive)
            int indirect_block;
            indirect_block = table->minodePtr->INODE.i_block[12];

            if(indirect_block == 0){ // if no indrect block, create it
                printf("my_write : Need to add an indirect datablock...\n");
                table->minodePtr->INODE.i_block[12] = indirect_block = make_indirect_block(dev); //assign indirect i_block

                if(indirect_block == -1){ //make indirect block error
                    printf("my_write unsuccessful\n");
                    return -1;
                }                
            }

            lbk -= 12; //since this is after the first 12 i_blocks, shift blk value

            d_block = get_indirect_block(dev, indirect_block, lbk); // get indirect block number

            if(d_block == -1){ //could not get data block from indirect block
                printf("my_write unsuccessful\n");
                return -1;
            }else if(d_block == 0){ // ADD NEW DATA BLOCK TO INDIRECT BLOCK
                // note: if get_indirect_block returns 0 -> it is an empty entry
                nblk = balloc(dev);
                
                if(!nblk){
                    printf("my_write unsuccessful\n");
                    return -1;
                }

                if(add_indirect_entry(dev, indirect_block, nblk) == -1){ // add nblk entry to data blocks
                    printf("my_write unsuccessful\n");
                    return -1;
                }
                printf("my_write : allocating a new datablock (indirectly) for file...\n");
                d_block = nblk;

            }

        }else if(lbk >= (BLKSIZE / 4) + 12){ // DOUBLE INDIRECT BLOCK
            // NOTE: For floppy disk lbk would be greater than 268 (inclusive)
            
            
            int double_indirect_block;
            double_indirect_block = table->minodePtr->INODE.i_block[13];

            if(double_indirect_block == 0){ // if no double indrect block, create it
                printf("my_write : Need to add a double indirect datablock...\n");

                // print next block ================
                char temp[BLKSIZE];
                int chunk;
                if(nbytes < remain){
                    chunk = nbytes; // if bytes left to write does not expand entire data block -> chunk is nbytes
                }else{
                    chunk = remain; // if the rest of data block can be written and still remaining nbytes -> chunk is remain
                }
                strncpy(temp, buf + buf_loc, chunk);
                printf("\n\n%s\n\n", temp);
                // =================================

                table->minodePtr->INODE.i_block[13] = double_indirect_block = make_indirect_block(dev); //assign indirect i_block
                // Note: We can still use the indirect function here because both indirect and double indirect blocks set all entries to int 0

                if(double_indirect_block == -1){ //make indirect block error
                    printf("my_write unsuccessful\n");
                    return -1;
                }  
            }

            lbk -= (BLKSIZE / 4) + 12; //since this is after the first 12 i_blocks and indirect blocks, shift blk value

            finding_block: //goto statement in case get_double_indirect_block fails
            d_block = get_double_indirect_block(dev, double_indirect_block, lbk); // get direct block from double indirect block

            if(d_block == -1){ //could not get data block from double indirect block
                printf("my_write unsuccessful\n");
                return -1;
            }else if(d_block == -2){ // ADD NEW INDIRECT DATA BLOCK TO DOUBLE INDIRECT BLOCK
                // note: if get_double_indirect_block returns -2 -> it is an empty double indirect entry
                nblk = make_indirect_block(dev); // make a new indirect block

                
                if(!nblk){
                    printf("my_write unsuccessful\n");
                    return -1;
                }

                if(add_indirect_entry(dev, double_indirect_block, nblk) == -1){ // add nblk entry to data blocks
                    printf("my_write unsuccessful\n");
                    return -1;
                }
                printf("my_write : allocating a new indirect datablock to double indirect block...\n");
                
                goto finding_block; // try again

            }else if(d_block == 0){ // ADD NEW DATA BLOCK ENTRY TO INDIRECT DATA BLOCK WITHIN DOUBLE INDIRECT DATABLOCK
                // note: if get_double_indirect_block returns 0 -> The indirect block direct block entry was 0

                int indirect_block;
                int _diblk = lbk / (BLKSIZE / 4);
                indirect_block = get_indirect_block(dev, double_indirect_block, _diblk); // just get the indirect block number within the double indirect block
                nblk = balloc(dev); // allocate new data block

                if(!nblk){
                    printf("my_write unsuccessful\n");
                    return -1;
                }

                if(add_indirect_entry(dev, indirect_block, nblk) == -1){ // add nblk entry to data blocks
                    printf("my_write unsuccessful\n");
                    return -1;
                }

                printf("my_write : allocating a new datablock (double-indirectly) for file...\n");
                goto finding_block; // try again

            }
            // If this point is reached: The block within indirect block within the double indirect block was found

        }else{ // DIRECT BLOCK: still within the first 12 i_blocks

            d_block = table->minodePtr->INODE.i_block[lbk];

            if(d_block == 0){ // if i_blocks don't stretch that far, allocate new data block
                printf("my_write : allocating a new datablock for file...\n");
                nblk = balloc(dev); // get the newly allocated block number
                table->minodePtr->INODE.i_block[lbk] = nblk; // add new data block to i_block list
                d_block = nblk;
            }
        }

        // AT THIS POINT: d_block should have the next data block for file

        if(d_block == 0){
            printf("my_write : panic! d_block not set\n");
            printf("my_write unsuccessful\n");
            return -1;
        }

        get_block(dev, d_block, bbuf); // get data block
        cp = bbuf + start;

        int chunk;
        if(nbytes < remain){
            chunk = nbytes; // if bytes left to write does not expand entire data block -> chunk is nbytes
        }else{
            chunk = remain; // if the rest of data block can be written and still remaining nbytes -> chunk is remain
        }
        printf("\t-> writing %d bytes to block %d, [%d for this file]\n", chunk, d_block, saved_lbk);
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

int cp_pathname(char* pathname){
    int oino, pino;
    char second_pathname[128], new_basename[128], temp[128], dirname[128];

    MINODE* omip, *pmip;

    sscanf(pathname, "%s %s", temp, second_pathname);  //seperate the two pathnames
    strcpy(pathname, temp);

    printf("pathname1: %s\npathname2: %s\n", pathname, second_pathname);

    oino = getino(pathname); //get inode number for old file

    if(!oino){ //check old file exsits
        printf("cp_pathname : %s is not a valid path\ncp unsuccessful\n", pathname);
        return -1;
    }
    
    omip = iget(dev, oino); //get the inode from old file

    if(is_dir(omip) == 0){ //if 0 -> is a directory
        printf("cp_pathname : %s is a directory, must be a REG file type\ncp unsuccessful\n", pathname);
        return -1;
    }

    if(getino(second_pathname) > 0){ //if second pathname has an inode, that means new file already exists
        printf("cp_pathname: %s already exists\ncp unsuccessful\n", second_pathname);
        return -1;

    }

    if(getino(second_pathname) == -1){ //if second pathname is invalid
        printf("cp_pathname: %s is not a valid pathname\ncp unsuccessful\n", second_pathname);
        return -1;

    }


    //tokenize second pathname
    tokenize(second_pathname);
    
    //determine to start at root or cwd
    if(second_pathname[0] == '/'){
            strcpy(dirname, "/"); //dirname is root
        }else{  //dirname is cwd
            strcpy(dirname, "");
    }

    //build the dirname
    for(int i = 0; i < (n-1); i++){
        strcat(dirname, name[i]);

        if((i+1) < (n-1)){
            strcat(dirname, "/");
        }
    }
    

    strcpy(new_basename, name[n - 1]); //get the _basename from path

    printf("dirname: %s\nbasename: %s\n", dirname, new_basename);
    
    if(strlen(dirname)){ //if a dirname was given
        pino = getino(dirname); //get the inode number for the parent directory

        if(!pino){ //dirname does not exist
            printf("cp unsuccessful\n");
            return -1;
        }

        //dirname is legit, now check if it is a directory
        pmip = iget(dev, pino); //create new minode for parent directory

    }else{ //if in cwd
        pmip = iget(dev, running->cwd->ino); //pmip becomes the cwd inode
    }


    if(is_dir(pmip) != 0){ //if given dirname is not a directory
    
        iput(pmip);
        printf("cp_pathname : %s is not a dir path\ncp unsuccessful\n", dirname);
        return -1;
        
    }
    // AT THIS POINT IN THE CODE: parent directory found, old file inode found, and new file name identified
    // all potential errors accounted for: safe to run my cp

    return my_cp(pathname, new_basename);
}

int my_cp(char* src, char* dest){

    int fd, gd, n, m; // n is the bits read from src to dest, m is bits written to dest from src
    char buf[BLKSIZE];

    fd = my_open(src, R);   // open source file for mode read
    gd = my_open(dest, RW); // open destination file for read-write (will creat if not exist)


    while(n = my_read(fd, buf, BLKSIZE)){ // read src in block size chunks
        printf("\n");
        if(n == -1){ //error with my_read
            printf("cp : catastrophic error reading src\n");
            printf("cp unsuccessful\n");

            // try to close files
            my_close(fd);
            my_close(gd);
            return -1;

        }
        m = my_write(gd, buf, n); // write n bytes to dest
        if(m == -1 || m == 0){ //error with my_write
            printf("cp : catastrophic error writing to dest\n");
            printf("cp unsuccessful\n");

            // try to close files
            my_close(fd);
            my_close(gd);
            return -1;

        }
    }

    // try to close files
    my_close(fd);
    my_close(gd);
    return 0;

}