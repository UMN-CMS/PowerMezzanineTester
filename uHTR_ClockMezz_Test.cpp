#include <cstdlib>
#include <cstdio>
#include <unistd.h> 
#include <time.h>
#include "uHTRClockMezzInterface.h"

void usage() {
  printf(" -m  Print the MAC Address \n");
  printf(" -c [value] Set the CSR register \n");
  printf(" -C Read the CSR register \n");
  printf(" -f Get firmware version\n");
  printf(" -r Get the clock rates/frequencies\n");
  printf(" -e [which] Read the EEPROM\n");
  printf(" -A [PLL file] Program the A EEPROM\n");
  printf(" -B [PLL file] Program the B EEPROM\n");
}

int main(int argc, char* argv[]) {

  uHTRClockMezzInterface clockMod(0);
  clockMod.setMUXChannel(2);
  //  clockMod.testBoardSetResets(true,true);
  clockMod.testBoardSetResets(false,false);

  if (argc==1) usage();

  int opt,which;
  std::vector<uint8_t> buffer;

  while ((opt = getopt(argc,argv,"mCfrc:e:A:B:D:")) != -1) {
    switch (opt) {
    case 'm' : 
      clockMod.readMezzMAC();
      clockMod.printMezzMAC();
      break;
    case 'c' : 
      clockMod.setCSR(strtol(optarg,0,0));
      break;
    case 'C' : 
      printf(" CSR = 0x%02x\n",clockMod.getCSR()&0xFF);
      break;
    case 'A' :
      clockMod.programSiLabsEEPROM(0,optarg);
      break;
    case 'B' :
      clockMod.programSiLabsEEPROM(1,optarg);
      break;
    case 'D' :
      clockMod.programSiLabsDirect(optarg);
      break;
    case 'r' : 
      printf(" ClockA = %.0f\n",clockMod.readClockFreq(0));
      printf(" ClockB = %.0f\n",clockMod.readClockFreq(1));
      break;
    case 'f' : 
      printf(" FW version = 0x%02x\n",clockMod.getFWVersion());
      break;
    case 'e' :
      which=atoi(optarg);
      buffer.clear();
      clockMod.readSiLabsEEPROM(which,buffer);
      for (unsigned int i=0; i<buffer.size(); i+=4) {
	printf("%4d %02x %02x %02x %02x\n",i,buffer[i],buffer[i+1],buffer[i+2],buffer[i+3]);
      }
      break;
    }
  }
  //  clockMod.programSiLabsEEPROM(0,"/home/jmmans/mswin/doc/b320.txt");


  /*

  printf("Firmware version=%d\n",clockMod.getFWVersion());
    printf("5319A Rate=%f\n",clockMod.readClockFreq(0));
  for (int i=0; i<10; i++) 

  printf("5319B Rate=%f\n",clockMod.readClockFreq(1));
  */

  return 0;
}
