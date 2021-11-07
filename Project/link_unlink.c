/***************** link_unlink.c *****************/

// Function declarations *************
int my_link(MINODE* pmip, int oino, char* _basename);
// ***********************************

extern int getino(char *pathname);
extern int is_dir(MINODE* mip);

extern int n;
extern char *name[64];


int link_pathname(char* pathname){

    int oino, pino;
    char second_pathname[128], new_basename[128], temp[128], dirname[128];

    MINODE* omip, *pmip;

    sscanf(pathname, "%s %s", temp, second_pathname);  //seperate the two pathnames
    strcpy(pathname, temp);

    printf("pathname1: %s\npathname2: %s\n", pathname, second_pathname);

    oino = getino(pathname); //get inode number for old file

    if(!oino){
        printf("link_pathname : %s is not a valid path\nlink unsuccessful\n", pathname);
        return -1;
    }
    
    omip = iget(dev, oino); //get the inode from old file

    if(is_dir(omip) == 0){ //if 0 -> is a directory
        printf("link_pathname : %s is a directory, must be a REG file type\nlink unsuccessful\n", pathname);
        return -1;
    }

    if(getino(second_pathname)){ //if second pathname has an inode, that means new file already exists
        printf("link_pathname: %s already exists\nlink unsuccessful\n", second_pathname);
        return -1;

    }
    //tokenize second pathname
    tokenize(second_pathname);
    
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
    

    strcpy(new_basename, name[n - 1]); //get the _basename from path

    printf("dirname: %s\nbasename: %s\n", dirname, new_basename);

    if(strlen(dirname)){ //if a dirname was given
        pino = getino(dirname); //get the inode number for the parent directory

        if(!pino){ //dirname does not exist
            printf("creat unsuccessful\n");
            return -1;
        }

        //dirname is legit, now check if it is a directory
        pmip = iget(dev, pino); //create new minode for parent directory
    }else{ //if in cwd
        pmip = iget(dev, running->cwd->ino); //pmip becomes the cwd inode
    }

    //parent directory found, old file inode found, and new file name identified
    //safe to run my link
    return my_link(pmip, oino, new_basename);
}

int my_link(MINODE* pmip, int oino, char* _basename){

}