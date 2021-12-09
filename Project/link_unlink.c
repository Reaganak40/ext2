/***************** link_unlink.c *****************/

// Function declarations *************
int my_link(MINODE* pmip, int oino, char* _basename);
// ***********************************

extern int getino(char *pathname);
extern int is_dir(MINODE* mip);
extern int enter_name(MINODE* pip, int ino, char* name);
extern int rm_child(MINODE* pmip, char* fname);
extern int bdalloc(int dev, int blk);
extern int idalloc(int dev, int ino);

extern int n;
extern char *name[64];

/*****************************************************
*
*  Name:    unlink_pathname
*  Made by: Reagan Kelley
*  Details: verifies that pathname given for unlink is 
*           valid, then does unlink implementation
* 
*****************************************************/
int unlink_pathname(char* pathname){
    int odev = dev; //keep track of original dev
    int ino, pino;
    MINODE* mip, *pmip;
    char dirname[128], _basename[128];

    ino = getino(pathname);
    
    if(!ino){ //if pathname does not exist
        printf("unlink pathname: %s does not exist.\nunlink unsuccessful\n", pathname);
        dev = odev; //reset back to original device
        return -1;
    }

    mip = iget(dev, ino); //get inode of pathname
    //printf("NEW mip: %d, dev %d, ref count: %d\n", mip->ino, mip->dev, mip->refCount);
    
    if(is_dir(mip) == 0){ //if mip is directory, dont unlink
        printf("unlink pathname: %s is a directory.\nunlink unsuccessful\n", pathname);
        iput(mip);
        dev = odev; //reset back to original device

        return -1;
    }
    mip->refCount++; // is_dir will derefrence minodes if fails, this usually works out, but want to continue to use it
    tokenize(pathname);
    
    //determine to start at root or cwd
    if(pathname[0] == '/'){
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
    

    strcpy(_basename, name[n - 1]); //get the _basename from path

    printf("dirname: %s\nbasename: %s\n", dirname, _basename);

    if(strlen(dirname)){ //if a dirname was given
        dev = odev; //reset back to original device
        pino = getino(dirname); //get the inode number for the parent directory

        if(!pino){ //dirname does not exist
            printf("unlink unsuccessful\n");
            dev = odev; //reset back to original device
            return -1;
        }

        //dirname is legit, now check if it is a directory
        pmip = iget(dev, pino); //create new minode for parent directory
    }else{ //if in cwd
        pmip = iget(dev, running->cwd->ino); //pmip becomes the cwd inode
    }
    //printf("NEW pmip: %d, dev %d, ref count: %d\n", pmip->ino, pmip->dev, pmip->refCount);


    if(rm_child(pmip, _basename) != 0){
        return -1;
    }

    pmip->dirty = 1;

    if((pmip->ino != mip->ino) || (pmip->dev != mip->dev)){ //only put if they are different
        iput(pmip);
    }

    //decrement inode's link count by 1
    mip->INODE.i_links_count--;
    if(mip->INODE.i_links_count > 0){ //if inode is still linked somewhere else
        mip->dirty = 1;
    }else{ //if inode is not linked anywhere else
        printf("No more links, removing all references...\n");
        for(int i = 0; i < 12; i++){ //assume no indirect i_blocks
            if(mip->INODE.i_block[i] != 0){
                bdalloc(dev, mip->INODE.i_block[i]); //deallocate the data block used by this file
            }else{ //no more allocated i_blocks
                break;
            }

        }

        idalloc(dev, mip->ino); //deallocate the inode used by this file
        mip->dirty = 1;

    }

    iput(mip); //release mip
    dev = odev; //reset back to original device

    return 0;
}

/*****************************************************
*
*  Name:    link_pathname
*  Made by: Reagan Kelley
*  Details: verifies that pathname given for link is 
*           valid, then prepares variables for my_link 
*           call
* 
*****************************************************/
int link_pathname(char* pathname){

    int oino, pino;
    char second_pathname[128], new_basename[128], temp[128], dirname[128];

    MINODE* omip, *pmip;

    sscanf(pathname, "%s %s", temp, second_pathname);  //seperate the two pathnames
    strcpy(pathname, temp);

    printf("pathname1: %s\npathname2: %s\n", pathname, second_pathname);

    oino = getino(pathname); //get inode number for old file

    if(!oino){ //check old file exsits
        printf("link_pathname : %s is not a valid path\nlink unsuccessful\n", pathname);
        return -1;
    }
    
    omip = iget(dev, oino); //get the inode from old file

    if(is_dir(omip) == 0){ //if 0 -> is a directory
        printf("link_pathname : %s is a directory, must be a REG file type\nlink unsuccessful\n", pathname);
        return -1;
    }

    if(getino(second_pathname) > 0){ //if second pathname has an inode, that means new file already exists
        printf("link_pathname: %s already exists\nlink unsuccessful\n", second_pathname);
        return -1;

    }

    if(getino(second_pathname) == -1){ //if second pathname is invalid
        printf("link_pathname: %s is not a valid pathname\nlink unsuccessful\n", second_pathname);
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
            printf("link unsuccessful\n");
            return -1;
        }

        //dirname is legit, now check if it is a directory
        pmip = iget(dev, pino); //create new minode for parent directory

    }else{ //if in cwd
        pmip = iget(dev, running->cwd->ino); //pmip becomes the cwd inode
    }


    if(is_dir(pmip) != 0){ //if given dirname is not a directory
    
        iput(pmip);
        printf("link_pathname : %s is not a dir path\nlink unsuccessful\n", dirname);
        return -1;
        
    }
    // AT THIS POINT IN THE CODE: parent directory found, old file inode found, and new file name identified
    // all potential errors accounted for: safe to run my link
    int success;
    success = my_link(pmip, oino, new_basename); //link file

    if(success == -1){ //my_link did not work
        return success;
    }

    omip->INODE.i_links_count++; //increment omip's inode link count by 1;
    omip->dirty = 1;
    
    iput(omip); //update old file's inode
    iput(pmip); //update parent inode (only happens if new i_block had to be allocated)

    return 0;



}

/*****************************************************
*
*  Name:    my link
*  Made by: Reagan Kelley
*  Details: Takes a parent directory, and makes a 
*           new link connected by the given inode, 
*           and then puts in that parent directory.
* 
*****************************************************/
int my_link(MINODE* pmip, int oino, char* _basename){
    return enter_name(pmip, oino, _basename);
}