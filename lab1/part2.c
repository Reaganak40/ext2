
  /* sample code for Part 2 */      
#include <stdio.h>
#include <fcntl.h>

#include <sys/types.h>
#include <unistd.h>

typedef unsigned int u32;
typedef unsigned char  u8;
typedef unsigned short u16;

struct partition {
	u8 drive;             // drive number FD=0, HD=0x80, etc.

	u8  head;             // starting head 
	u8  sector;           // starting sector
	u8  cylinder;         // starting cylinder

	u8  sys_type;         // partition type: NTFS, LINUX, etc.

	u8  end_head;         // end head 
	u8  end_sector;       // end sector
	u8  end_cylinder;     // end cylinder

	u32 start_sector;     // starting sector counting from 0 
	u32 nr_sectors;       // number of of sectors in partition
  };


char *dev = "vdisk";
int fd;
char buf[512];
    
// read a disk sector into char buf[512]
int read_sector(int fd, int sector, char *buf)
{
    lseek(fd, sector*512, SEEK_SET);  // lssek to byte sector*512
    read(fd, buf, 512);               // read 512 bytes into buf[ ]
}

void extend_partition(struct partition *p, u32 ext_start, u32 localMBR, int partition_number){ //recursive function to read 'linked-list' partition
  
  if(p->start_sector == 0){ //end of linked-list
    return;
  }

  read_sector(fd, ext_start, buf);
  p = (struct partition *)(&buf[0x1be]);

  // partition table of localMBR in buf[ ] has 2 entries: 
  //print entry 1's start_sector, nr_sector;
  printf("------------------------------------------\n");
  printf("Entry1: start_sector: %d   nr_sectors: %d\n", p->start_sector,p->nr_sectors);
  p++;
  printf("Entry2: start_sector: %d   nr_sectors: %d\n", p->start_sector,p->nr_sectors);
  printf("------------------------------------------\n");

  p--;

  printf("      start Sector  end_sector  nr_sectors\n");  
  printf("P%d : %8d  %10d  %12d\n\n", partition_number, p->start_sector + ext_start, (p->start_sector + ext_start + p->nr_sectors - 1), p->nr_sectors);

  
  p++;
  u32 newStart = localMBR + (p->start_sector);
  
  if(p->start_sector == 0){
    printf("End of Extend Partitions\n");
  }else{
    printf("next localMBR = %d + %d = ", localMBR, p->start_sector);
    
    printf("%d\nnext localMBR = %d\n", newStart, newStart);
  }
  extend_partition(p, newStart, localMBR, ++partition_number);
  


}

void run_ptable(void){
  struct partition *p, *q;

  fd = open(dev, O_RDONLY);   // open dev for READ
  read_sector(fd, 0, buf);    // read in MBR at sector 0    

  p = (struct partition *)(&buf[0x1be]); // p->P1

  q = p;
  char input[100];

  printf("      start Sector  end_sector  nr_sectors\n");  
  for(int i = 1; i <= 4; i++){ //print partition table
    printf("P%d : %8d  %10d  %12d\n", i, p->start_sector, (p->start_sector + p->nr_sectors - 1), p->nr_sectors);
    
    
    p++;
  }
  scanf("%[^\n]", input);


  p = q;
  printf(" ***************** Look for Extend Partition *****************\n");
  for(int i = 1; i <= 4; i++){ //print partition table
    printf("[%d 0%d] ", i, p->sys_type);

    if(p->sys_type == 5){
      break;
    }
    p++;
  } 
  printf("\n"); 

  // ASSUME P4 is EXTEND type:
  //Let  int extStart = P4's start_sector;
  int extStart = p->start_sector; 
  
  //print extStart to see it;
  printf("P4's start_sector: %d\n", extStart);

  

  //localMBR = extStart;
  int localMBR = extStart;
  //loop:
  extend_partition(p, extStart, localMBR, 5);

}
int main()
{

  run_ptable();

  return 0;
  
  //compute and print P5's begin, end, nr_sectors
  /*
  if (entry 2's start_sector != 0){
     compute and print next localMBR sector;
     continue loop;
  }
  */
}

