#include <stdio.h>
#include <stdarg.h>


typedef unsigned int u32;


char *ctable = "0123456789ABCDEF";
int  BASE = 10; 

int rpu(u32 x)
{  
    char c;
    if (x){
       c = ctable[x % BASE];
       rpu(x / BASE);
       putchar(c);
    }
}

int printu(u32 x)
{
   (x==0)? putchar('0') : rpu(x);
   //putchar(' ');
}

int prints(char* string){
    
    char* loc = string; //local variable locates string index

    while((*loc) != '\0'){ //while char is not terminating character 

        putchar((*loc));
        loc += 1;

    }
    return 1;
}


int printd(int x){
    if(x < 0){ //if x is negative

        putchar('-'); //preprints negative symbol before reading the int
        x *= -1; //makes x positive
    }

    rpu(x); //prints positive x

    return 1;
}

int printx(u32 x){

    BASE = 16; //change to hex base
    
    putchar('0'); //0x
    putchar('x'); //0x
    rpu(x); //will print in hex for when base is 16

    BASE = 10; //change back to deciamal base (to avoid future computational errors)



    return 1;
}

int printo(u32 x){

    BASE = 8; //change to octal base
    
    putchar('0'); //0
    rpu(x); //will print in hex for when base is 16

    BASE = 10; //change back to deciamal base (to avoid future computational errors)

    return 1;
}

int myprintf(char* fmt, ...){

    char* cp= fmt;
    char c = *cp;
    int alert = 0;

    int* ip = (int*)&fmt+1;

   
    while(c != '\0'){ //while char is not terminating character 

        if(alert){ //if previous read char was '%'

            switch(c){ //call print function based on %_
                case 'c':
                    putchar(*ip);
                break;
                case 's':
                    prints((char*)*ip);
                break;
                case 'u':
                    printu((u32)*ip);
                break;
                case 'd':
                    printd((int)*ip);
                break;
                case 'o':
                    printo((int)*ip);
                break;
                case 'x':
                    printx((int)*ip);
                break;
                default:
                    putchar(c); //could not interpret %_
                break;

            }

            ip++; //go to next variable in the stack
            alert = 0; //reset alert for variable
        }else{

            if(c == '%'){ //raise alert to print variable in next iteration
                alert = 1;
            }else{
                putchar(c);

                if(c == '\n'){
                    putchar('\r');
                }
            }


        }

        cp += 1; //go to next character in the main string
        c = *cp; //set c to char at string index
        

    }
    


    return 1;
}



int main(int argc, char *argv[ ], char **env[ ]){

    myprintf("Number of arguments: %d\n", argc);

    myprintf("------------\n");
    for(int i = 0; i < argc; i++){
        myprintf("%s\n", *argv);
        argv++;
    }
    myprintf("------------\n");
    while(*env != NULL){
        myprintf("%s\n", *env);
        env++;
    }
    myprintf("------------\n");




    prints("noah");
    printf("\n");
    printd(-67);
    printf("\n");
    printx(1684);
    printf("\n");
    printo(8876);
    printf("\n");
    myprintf("test: %c %s %u %d %o %x\n", 'n', "candy", 101, 67, 9, 45);
    myprintf("cha=%c string=%s      dec=%d hex=%x oct=%o neg=%d\n", 
            'A', "this is a test", 100,    100,   100,  -100);

    
    return 0;
}