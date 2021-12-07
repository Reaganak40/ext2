/************* mount_umount.c file **************/

int talloc(int ndev, int table_num);


extern int my_mkdir(MINODE* pmip, char* _basename);

MOUNT *getmptr(int dev){

    for(int i = 0; i < NMOUNT; i++){
        if (mountTable[i].dev == dev){
            return &mountTable[i];
        }
    }

}

int my_mount(char* pathname){

    if(strlen(pathname) == 0){ // if no parameter: print mounted systems
        printf("listing mounted systems...\n\n");
        for(int i = 0; i < NMOUNT; i++){
            if (mountTable[i].dev != 0){
                printf("%s on %s\n", mountTable[i].name, mountTable[i].mount_name);

                printf("\t dev = %d (EXT2)\n", mountTable[i].dev); //for this project all mounted systems are EXT2
                printf("\t ninodes = %d\n", mountTable[i].ninodes);
                printf("\t nblocks = %d\n", mountTable[i].nblocks);
                printf("\t iblk = %d\n", mountTable[i].iblk);
            }
        }
        printf("\n");
        return 0;
    }

    int pino;
    char second_pathname[128], new_basename[128], temp[128], dirname[128];

    MINODE *pmip;

    sscanf(pathname, "%s %s", temp, second_pathname);  //seperate the two pathnames
    strcpy(pathname, temp);

    printf("pathname1: %s\npathname2: %s\n", pathname, second_pathname);

    if(getino(second_pathname) == -1){ //if second pathname is invalid
        printf("mount : %s is not a valid pathname\nmount unsuccessful\n", second_pathname);
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
            printf("mount unsuccessful\n");
            return -1;
        }

        //dirname is legit, now check if it is a directory
        pmip = iget(dev, pino); //create new minode for parent directory

    }else{ //if in cwd
        pmip = iget(dev, running->cwd->ino); //pmip becomes the cwd inode
    }


    if(is_dir(pmip) != 0){ //if given dirname is not a directory
    
        iput(pmip);
        printf("mount : %s is not a dir path\nmount unsuccessful\n", dirname);
        return -1;
        
    }

    // ===============================================================================
    // AT THIS POINT IN THE CODE: the second paramter has been verified as a safe
    // all potential errors accounted for: safe to run my mount
    // ===============================================================================
    int fd, ino;
    MINODE* mip;
    char buf[BLKSIZE];
    DIR *dp;
    char *cp;
    

    printf("preparing mount...\n");
    if((ino = getino(second_pathname)) > 0){ //if second pathname has an inode, that means new file already exists
        mip = iget(dev, ino);


    }else{ // if it doesn't exist, create it

        my_mkdir(pmip, new_basename); // create new directory where the mounted filesys will be
        ino = getino(second_pathname);

        if(!ino){
            printf("mount : could not get inode of mount directory\nmount unsuccessful\n");
            return -1;
        }
        mip = iget(dev, ino);

    }

    for(int i = 0; i < NMOUNT; i++){ // check that the filesys is not already mounted
        if(mountTable[i].dev != 0){
            if(strcmp(mountTable[i].name, pathname) == 0){ //pathname is the first parameter 'disk name'
                printf("mount : %s is already opened.\nmount unsuccessful\n", pathname);
                iput(mip);
                return -1;
            }
        }
    }

    //opens disk for read and write
    if ((fd = open(pathname, O_RDWR)) < 0){
        printf("open %s failed\nmount unsuccessful\n", pathname);
        iput(mip);

        return -1;
    }
    printf("dev: %d\n", fd);
    
    // safe to start mounting
    mip->mounted = 1;   // mount flag is TRUE

    


    int was_mounted = 0;
    for(int i = 0; i < NMOUNT; i++){ // allocate device to mount table
        if(mountTable[i].dev == 0){
           
            if(talloc(fd, i) == -1){ //allocate system information to table
                printf("mount unsuccessful\n");
                return -1;
            }
            
            strcpy(mountTable[i].name, pathname);
            strcpy(mountTable[i].mount_name, second_pathname);
            
            mip->mptr = &mountTable[i];
            mountTable[i].mounted_inode = mip;
            was_mounted = 1;
            break;
        }
    }

    if(!was_mounted){ //could not add disk to mount table
        printf("mount : could not mount disk, mountTable full\nmount unsuccessful\n");
        return -1;
    }


    return 0;
}

int talloc(int ndev, int table_num){

    char buf[BLKSIZE];

    /********** read super block  ****************/
    get_block(ndev, 1, buf); // BLOCK #1 is reserved for Superblock
    sp = (SUPER *)buf;      // sp (super-pointer) reads buf as a ext2_super_block

    /* verify it's an ext2 file system ***********/
    if (sp->s_magic != 0xEF53){ // 0xEF53 specifies that its an ext2 file system
        printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
        return -1;
    }     
    mountTable[table_num].ninodes = sp->s_inodes_count; //how many inodes are on the disk
    mountTable[table_num].nblocks = sp->s_blocks_count; //how many blocks are on the disk

    /********** Group Descriptor Block *************/
    get_block(ndev, 2, buf); //  BLOCK #2 is reserved for Group Descriptor Block
    gp = (GD *)buf;          //  gd (group descriptor pointer) reads buf as a ext2_group_desc

    mountTable[table_num].bmap  = gp->bg_block_bitmap; // bmap info received from group descriptor block
    mountTable[table_num].imap  = gp->bg_inode_bitmap; // imap info received from group descriptor block
    mountTable[table_num].iblk  = gp->bg_inode_table;  // iblk info received from group descriptor block
    mountTable[table_num].dev = ndev;

    return 0;

}