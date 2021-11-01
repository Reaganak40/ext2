#include <stdio.h>             // for I/O
#include <stdlib.h>            // for I/O
#include <libgen.h>            // for dirname()/basename()
#include <string.h>

int cd(char* name);
char* pwd(int print);

typedef struct node{
         char  name[64];       // node's name string
         char  type;           // 'D' for DIR; 'F' for file
   struct node *child, *sibling, *parent;

}NODE;


NODE *root, *cwd, *start;
char line[128];
char command[16], pathname[64];

//               0       1      2       3    4     5         6     7       8      9        10
char *cmd[] = {"mkdir", "ls", "quit", "cd", "?", "rmdir", "pwd", "creat", "rm", "save", "reload", 0};


int findCmd(char *command)
{
   int i = 0;

   while(cmd[i]){
     if (strcmp(command, cmd[i])==0)
         return i;
     i++;
   }
   return -1;
}

NODE *search_child(NODE *parent, char *name)
{

  NODE *p;
  //printf("search for %s in parent DIR\n", name);
  p = parent->child;
  if (p==0)
    return 0;
  while(p){
    if (strcmp(p->name, name)==0)
      return p;
    p = p->sibling;
  }
  return 0;
}

int insert_child(NODE *parent, NODE *q)
{
  NODE *p;
  //printf("insert NODE %s into END of parent child list\n", q->name);
  p = parent->child;
  if (p==0)
    parent->child = q;
  else{
    while(p->sibling)
      p = p->sibling;
    p->sibling = q;
  }
  q->parent = parent;
  q->child = 0;
  q->sibling = 0;
}

int remove_child(NODE* parent, NODE* q){
  NODE *p;
  p = parent->child;


  if (p==q) //if q is direct child of parent
    parent->child = q->sibling; //make parent's child q's sibling
  else{
    while(p->sibling){
      if(p->sibling == q){ //q is found in sibling tree
        p->sibling = q->sibling; //remove q from sibling tree
        break;
      }
      p = p->sibling;
    }
  }
  
  free(q); //free memory of pointer q;
}

void save_tree(NODE* p, FILE* myfile){
  char* wd;
  cwd = p;
  //save p type and name
  fputc(p->type, myfile);
  fputc(' ', myfile);

  wd = pwd(0); //0 to not print 

  fputs(wd, myfile);
  fputc('\n', myfile);

  if(p->child){ //if p has a child
    save_tree(p->child, myfile); //save child subtree
  }

  if(p->sibling){ //if p has a sibling
    save_tree(p->sibling, myfile); //save child subtree
  }
}

NODE* find_dir_node(char* name, int* last_ref){ 
    NODE* p;

    int str_start, str_end;
    char substring[128];

    str_start = 0;
    str_end = 0;


    if(name[0] == '/'){
      p = root;
      str_start++;
      str_end++;

    }else if(name[0] == '.'){
      if(name[1] == '.'){ // path is ..

          if(name[2] == '/'){ // cd ../
            if(cwd == root){ // if cd ../ at root
              return NULL;
            }
            p = cwd->parent;
            str_start += 3;
            str_end += 3;
          }else if(name[2] == '\0'){ // cd ..
            p = cwd->parent;
            str_start += 2;
            str_end += 2;
          }
      }else if(name[1] == '/'){ // path is ./
          str_start += 2;
          str_end += 2;
          p = cwd;
      }else if(name[1] == '\0'){ // cd .
        str_start++;
        str_end++;
        p = cwd; 
      }
      else{ // not special command ./ or ../
          p = cwd; //just resort to cwd
      }
    }
    else{
      p = cwd;
    }

    while(1){ //traverse pathname
      if(name[str_end] == '/' || name[str_end] == '\0'){

        strncpy(substring, name + str_start, (str_end - str_start));
        
        substring[str_end - str_start] = '\0';
        //printf("%s\n", substring);

        
        if(name[str_end] == '\0'){ //if at last directory name, return its position in pathname via pointer
          *last_ref = str_start;
          break;
        }

             
        p = search_child(p, substring);

        if(!p){ //child doesn't exist
          *last_ref = str_start;
          return NULL;
        }else if(p->type == 'F'){ //if node type is file (not a directory)
          *last_ref = str_start;
          return NULL; 
        }

        str_start = str_end + 1;
        str_end = str_start;

      }

      str_end++;
    }

    return p;
}

/***************************************************************
 This mkdir(char *name) makes a DIR in the current directory
 You MUST improve it to mkdir(char *pathname) for ANY pathname
****************************************************************/
int mkdir(char *name)
{
  NODE *p, *q;
  int last_ref = 0;

  //printf("mkdir: name=%s\n", name);

  if (!strcmp(name, "/") || !strcmp(name, ".") || !strcmp(name, "..")){
    printf("can't mkdir with %s\n", name);
    return -1;
  }

  //making mkdir work for any pathname
  start = find_dir_node(name, &last_ref);

  if(start == NULL){ //couldn't track pathname
    printf("cant find %s\n", (name + last_ref));
    return -1;
  }
  

  //printf("check whether %s already exists\n", name);
  p = search_child(start, name);
  if (p){
    printf("name %s already exists, mkdir FAILED\n", name);
    return -1;
  }

  
  //printf("--------------------------------------\n");
  //printf("ready to mkdir %s\n", name);
  q = (NODE *)malloc(sizeof(NODE));
  q->type = 'D';
  strcpy(q->name, name + last_ref);
  insert_child(start, q);
  printf("mkdir %s OK\n", name);
  //printf("--------------------------------------\n");
    
  return 0;
}

int rmdir(char *name){
  NODE *p;
  int last_ref = 0;

  if (!strcmp(name, "/") || !strcmp(name, ".") || !strcmp(name, "..")){
    printf("can't rmdir with %s\n", name);
    return -1;
  }

  //making rmdir work for any pathname
  start = find_dir_node(name, &last_ref);

  if(start == NULL){ //couldn't track pathname
    printf("cant find %s\n", (name + last_ref));
    return -1;
  }

  p = search_child(start, name + last_ref);

  if (!p){ //if target dir doesn't exist
    printf("name %s doesn't exist, rmdir FAILED\n", name + last_ref);
    return -1;
  }

  if(p->type == 'F'){ //if target dir is actuslly a file
    printf("%s is not a directory, rmdir FAILED\n", name + last_ref);
    return -1;
  }
  if((p->child)){ //if dir not empty
    printf("name %s is not empty, rmdir FAILED\n", name + last_ref);
    return -1;
  }

  
  //remove directory from directory tree
  remove_child(start, p);


}

// This ls() list CWD. You MUST improve it to ls(char *pathname)
int ls(char* name)
{

  NODE *p;
  int last_ref = 0;

  if(strcmp(name, root->name) == 0){
    p = root->child;
    
  }else if(strcmp(name, ".") == 0 || strcmp(name, "./") == 0){ // ls .
    p = cwd->child;
  }else if(strcmp(name, "..") == 0 || strcmp(name, "../") == 0){ // cd ..
    if(cwd == root){ // when cd .. at root
      printf("can't track path .. when cwd is root\n");
      return 0;
    }else{
      p = cwd->parent->child;
    }
  }else{
  
    if(strlen(name) == 0){
      p = cwd->child;

    }else{ //if ls has pathname included
      p = find_dir_node(name, &last_ref);
      p = search_child(p, name + last_ref);

      if(!p){ //if pathname doesn't work
        printf("can't find path for ls with [...]/%s\n", name + last_ref);
        return 0;
      }
      p = p->child;
    }
  }
  printf("cwd contents = ");
  while(p){
    printf("[%c %s] ", p->type, p->name);
    p = p->sibling;
  }
  printf("\n");
}

// This cd() changes directory.
int cd(char* name)
{
  

  if(strcmp(name, root->name) == 0){
    cwd = root;
    return 1;
  }else if(strcmp(name, ".") == 0){ //cd .
    return 1; // cd to cwd
  }else if(strcmp(name, "..") == 0){ // cd ..
    if(cwd == root){ // when cd .. at root
      printf("can't track path .. when cwd is root\n");
      return 0;
    }else{
      cwd = cwd->parent;
      return 1;
    }
  }

  NODE *p, *q;
  int last_ref = 0;
  printf("cd: path = [%s]\n", name);

  p = cwd;
  cwd = find_dir_node(name, &last_ref);

  if(cwd == NULL){ //if pathname doesn't work
    printf("can't find path for cd with %s\n", name + last_ref);
    cwd = p;
    return 0;
  }

  p = search_child(cwd, name + last_ref);

 
  if(!p){ //if cd name doesn't exist
      printf("can't find path for cd with %s\n", name + last_ref);
      return 0;
  }else if(p->type == 'F'){ //if target is not a directory
    printf("%s is not a directory, cd FAILED\n", name + last_ref);
    return -1; 
  }
  else{ //change cwd to cd name
    cwd = p;
    printf("---- cd OK ----\n");
    return 1;
  }

}

char* pwd(int print){
  NODE *p;
  char* list_of_dir[100];
  char wd[200];
  char* to_return;

  p = cwd;
  int n = 0; //nth directory found in path

  while(p != root){
    list_of_dir[n] = p->name;
    p = p->parent;
    n++;
  }
  if(print)
  printf("**** pwd *****\n");

  strcpy(wd, "");

  if(n == 0){
    if(print)
    printf("/\n");
    strcpy(wd, "/");
    to_return = wd;
    return to_return;
  }

  for(int i = (n-1); i >= 0; i--){
    strcat(wd, "/");
    strcat(wd, list_of_dir[i]);

  }

  if(print)
  printf("%s\n", wd);

  to_return = wd;
  return to_return;
}

int creat(char* name){

  NODE *p, *q;
  int last_ref = 0;

  //printf("creat: name=%s\n", name);

  if (!strcmp(name, "/") || !strcmp(name, ".") || !strcmp(name, "..")){
    printf("can't creat with %s\n", name);
    return -1;
  }

  //making mkdir work for any pathname
  start = find_dir_node(name, &last_ref);

  if(start == NULL){ //couldn't track pathname
    printf("cant find %s\n", (name + last_ref));
    return -1;
  }
  

  p = search_child(start, name + last_ref);

  if (p){
    printf("file name %s already exists, creat FAILED\n", name + last_ref);
    return -1;
  }
  //printf("--------------------------------------\n");
  //printf("ready to creat %s\n", name);
  q = (NODE *)malloc(sizeof(NODE));
  q->type = 'F';
  strcpy(q->name, name + last_ref);
  insert_child(start, q);
  printf("creat %s OK\n", name);
  //printf("--------------------------------------\n");
    
  return 0;
}

int rm(char* name){
  NODE *p;
  int last_ref = 0;

  if (!strcmp(name, "/") || !strcmp(name, ".") || !strcmp(name, "..")){
    printf("can't rm with %s\n", name);
    return -1;
  }

  //making rm work for any pathname
  start = find_dir_node(name, &last_ref);

  if(start == NULL){ //couldn't track pathname
    printf("cant find %s\n", (name + last_ref));
    return -1;
  }

  p = search_child(start, name + last_ref);

  if (!p){ //if target dir doesn't exist
    printf("name %s doesn't exist, rm FAILED\n", name + last_ref);
    return -1;
  }

  if(p->type == 'D'){ //if target file is actually a directory
    printf("%s is not a file, rm FAILED\n", name + last_ref);
    return -1;
  }

  //remove directory from directory tree
  remove_child(start, p);

}

void save(void){
  FILE* myfile;
  myfile = fopen("myfile", "w");

  NODE*p;
  p = cwd;
  save_tree(root, myfile);
  cwd = p;

  fclose(myfile);

}

void reload(void){
  FILE* myfile;
  myfile = fopen("myfile", "r");

  char buff[200];

  char type;

  while(!feof(myfile)){


    fgets(buff, 200, myfile); //scan line and check for feof
    if(feof(myfile)){
      break;
    }

    //get file information
    type = buff[0];

    for(int i = 2; i <= 200; i++){
      if(buff[i] == '\n' || buff[i] == '\0'){
        buff[i] = '\0';
        break;
      }
    }
    //printf("%s\n", buff + 2);

    

    if(type == 'D'){
      if(strcmp("/", buff + 2) != 0){
        mkdir(buff + 2);
      }
    }else if(type == 'F'){
      creat(buff + 2);
    }else{
      printf("Error: Issues reading file\n");
    }

  }

  fclose(myfile);

}

int quit()
{
  printf("Program exit\n");
  save();
  exit(0);
  // improve quit() to SAVE the current tree as a Linux file
  // for reload the file to reconstruct the original tree
}

//print help menu
void help(void){
  printf("====================== MENU ======================\n");
  printf(" mkdir rmdir ls cd pwd creat rm save reload quit \n");
  printf("==================================================\n");

}


int initialize()
{
    root = (NODE *)malloc(sizeof(NODE));
    strcpy(root->name, "/");
    root->parent = root;
    root->sibling = 0;
    root->child = 0;
    root->type = 'D';
    cwd = root;
    printf("Root initialized OK\n");
}

int main()
{
  int index;
  
  initialize();

  printf("Enter ? for help menu\n");

  while(1){
      printf("Command : ");
      fgets(line, 128, stdin);
      line[strlen(line)-1] = 0;

      command[0] = pathname[0] = 0;
      sscanf(line, "%s %s", command, pathname);
      //printf("command=%s pathname=%s\n", command, pathname);
      
      if (command[0]==0) 
         continue;

      index = findCmd(command);
      printf("**** %s %s\n", command, pathname);
      switch (index){
        case 0: mkdir(pathname);    break;
        case 1: ls(pathname);               break;
        case 2: quit();             break;
        case 3: cd(pathname);       break;
        case 4: help();             break;
        case 5: rmdir(pathname);    break;
        case 6: pwd(1);             break;
        case 7: creat(pathname);    break;
        case 8: rm(pathname);       break;
        case 9: save();             break;
        case 10: reload();          break;


        default: printf("invalid command\n");
          break;

      }
  }
}