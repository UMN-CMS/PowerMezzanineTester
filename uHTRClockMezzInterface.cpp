#include "uHTRClockMezzInterface.h"
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>

// i2c slave addresses
#define I2C_SADDRESS_BASEMUX   (0x70)
#define I2C_SADDRESS_SPIBRIDGE (0x28)
#define I2C_SADDRESS_U4_IO     (0x20)
			       	    
// SPI targets		       	    
#define SPI_ADDR_CPLD          (0x01)
#define SPI_ADDR_MAC           (0x02)
#define SPI_ADDR_EEPROM        (0x04)
#define SPI_ADDR_5319A         (0x08)
			       	    
#define SILABS_CMD_SETADDRESS  (0x00)
#define SILABS_CMD_WRITE       (0x40)
			       	    
#define EEPROM_READ            (0x03)
#define EEPROM_WRITE           (0x02)
#define EEPROM_WREN            (0x06)
#define EEPROM_RDSR            (0x05)

#define CSR_START_MEASURE      (0x01)
#define CSR_WHICH_MEASURE      (0x02)
#define CSR_START_PROG_5319A   (0x04)
#define CSR_START_PROG_5319B   (0x08)
#define CSR_LED2               (0x80)

#define GPIO_READ              (0x00)
#define GPIO_WRITE             (0x01)
#define GPIO_DIRECTION         (0x03)
#define GPIO_RESET_5319A       (0x80)
#define GPIO_RESET_5319B       (0x10)


uHTRClockMezzInterface::uHTRClockMezzInterface(int idev) : uHTRMezzInterface(idev, false) 
{

}
int uHTRClockMezzInterface::readMezzMAC() {
  if(!openSuccessful_) return 999;

  MAC_[0] = MAC_[1] = MAC_[2] = MAC_[3] = MAC_[4] = MAC_[5] = 0x00;

  char cmdString[100];
  cmdString[0]=SPI_ADDR_MAC;
  cmdString[1]=EEPROM_READ;
  cmdString[2]=0xFA;

  sub_i2c_write(sh_, I2C_SADDRESS_SPIBRIDGE, 0, 0, cmdString, 3+6);
  sub_i2c_read(sh_, I2C_SADDRESS_SPIBRIDGE, 0, 0, cmdString, 8); //MAC is 6 bytes long
  for (int i=0; i<6; i++)
    MAC_[i]=cmdString[i+2];
  
  errval_ = sub_errno;
  i2c_status_ = sub_i2c_status;
  return 0;
}
int uHTRClockMezzInterface::setCSR(uint8_t value) {
  if(!openSuccessful_) return 999;

  regWrite(0,value);

  return 0;
}

int uHTRClockMezzInterface::getCSR() {
  if(!openSuccessful_) return 999;

  return regRead(0);
}

uint8_t uHTRClockMezzInterface::regRead(int addr) {
  if(!openSuccessful_) return 0;

  char cmdString[3];
  cmdString[0]=SPI_ADDR_CPLD;
  cmdString[1]=addr&0x3;
  
  sub_i2c_write(sh_, I2C_SADDRESS_SPIBRIDGE, 0, 0, cmdString, 1+2);
  sub_i2c_write(sh_, I2C_SADDRESS_SPIBRIDGE, 0, 0, cmdString, 1+2);

  sub_i2c_read(sh_, I2C_SADDRESS_SPIBRIDGE, 0, 0, cmdString, 2);

  return uint8_t(cmdString[1]);
}

void uHTRClockMezzInterface::regWrite(int addr,uint8_t val) {
  if(!openSuccessful_) return;

  char cmdString[3];
  cmdString[0]=SPI_ADDR_CPLD;
  cmdString[1]=(addr&0x3)|0x80;
  cmdString[2]=val;
  
  sub_i2c_write(sh_, I2C_SADDRESS_SPIBRIDGE, 0, 0, cmdString, 1+2);

}


int uHTRClockMezzInterface::getFWVersion(){
  if(!openSuccessful_) return 999;

  return regRead(1);
}


int uHTRClockMezzInterface::programSiLabsEEPROM(int which, const char* fname) {

  uint8_t memblock[512];
  for (int i=0; i<512; i++) memblock[i]=0;
  int nwords=0;

  FILE* f=fopen(fname,"r");
  if (f==0) {
    fprintf(stderr,"Unable to open '%s'\n",fname);
    return 9998;
  }
  char buffer[80];
  int val1, val2;
  while (!feof(f)) {
    buffer[0]=0;

    fgets(buffer,80,f);

    if (buffer[0]=='#') continue;

    if (sscanf(buffer," %d, %xh",&val1,&val2)==2) {
      memblock[nwords]=SILABS_CMD_SETADDRESS;
      memblock[nwords+1]=val1;
      memblock[nwords+2]=SILABS_CMD_WRITE;
      memblock[nwords+3]=val2;
      nwords+=4;
    }    
  }
  fclose(f);

  printf("Words to write: %d\n",nwords);

  nwords=512;

  if(!openSuccessful_) return 999;

  char cmd[128];
  const int chunkSize=32;

  for (int ibase=0; ibase<nwords; ibase+=chunkSize) {
    printf("%d %d \n",ibase,nwords);
    cmd[0]=SPI_ADDR_EEPROM;
    cmd[1]=EEPROM_WREN;
    sub_i2c_write(sh_, I2C_SADDRESS_SPIBRIDGE, 0, 0, cmd, 1+1);

    
    cmd[0]=SPI_ADDR_EEPROM;
    cmd[1]=EEPROM_RDSR;
    sub_i2c_write(sh_, I2C_SADDRESS_SPIBRIDGE, 0, 0, cmd, 1+2);
    sub_i2c_read(sh_, I2C_SADDRESS_SPIBRIDGE, 0, 0, cmd, 1+2);
        printf("%x\n",cmd[1]&0xFF);
    

    cmd[0]=SPI_ADDR_EEPROM;
    cmd[1]=EEPROM_WRITE;
    cmd[2]=((which)?(0x80):(0))+((ibase>>8)&0xFF);
    cmd[3]=ibase&0xFF;
    printf("%x %x %x \n",ibase,cmd[2]&0xFF,cmd[3]&0xFF);
    for (int i=0; i<chunkSize; i++)
      cmd[4+i]=memblock[i+ibase];

    
    for (unsigned int i=0; i<chunkSize; i+=4) {
      printf("%4d %02x %02x %02x %02x\n",i,cmd[4+i]&0xFF,cmd[4+i+1]&0xFF,cmd[4+i+2]&0xFF,cmd[4+i+3]&0xFF);
    }
    

    sub_i2c_write(sh_, I2C_SADDRESS_SPIBRIDGE, 0, 0, cmd, 1+3+chunkSize);


    usleep(10000); // 5ms write cycle time
    
    cmd[0]=SPI_ADDR_EEPROM;
    cmd[1]=EEPROM_RDSR;
    sub_i2c_write(sh_, I2C_SADDRESS_SPIBRIDGE, 0, 0, cmd, 1+2);
    sub_i2c_read(sh_, I2C_SADDRESS_SPIBRIDGE, 0, 0, cmd, 1+2);
    printf("%d\n",cmd[1]);
  }

  std::vector<uint8_t> check;

  printf("Verifying...\n");
  readSiLabsEEPROM(which,check);
  for (int i=0; i<nwords; i++) {
    if (memblock[i]!=check[i]) printf("%03x : %02x %02x\n",i,memblock[i]&0xFF,check[i]&0xFF);
  }

  return 0;

}

int uHTRClockMezzInterface::programSiLabsDirect(const char* fname) {

  uint8_t memblock[512];
  for (int i=0; i<512; i++) memblock[i]=0;
  int nwords=0;

  FILE* f=fopen(fname,"r");
  if (f==0) {
    fprintf(stderr,"Unable to open '%s'\n",fname);
    return 9998;
  }
  char buffer[80];
  int val1, val2;
  while (!feof(f)) {
    buffer[0]=0;

    fgets(buffer,80,f);

    if (buffer[0]=='#') continue;

    if (sscanf(buffer," %d, %xh",&val1,&val2)==2) {
      memblock[nwords]=SILABS_CMD_SETADDRESS;
      memblock[nwords+1]=val1;
      memblock[nwords+2]=SILABS_CMD_WRITE;
      memblock[nwords+3]=val2;
      nwords+=4;
    }    
  }
  fclose(f);

  printf("Words to write: %d\n",nwords);

  if(!openSuccessful_) return 999;

  char cmd[128];

  int step=64;

  for (int ibase=0; ibase<nwords; ibase+=step) {
    cmd[0]=SPI_ADDR_5319A;
    for (int i=0; i<step; i++)
      cmd[i+1]=memblock[ibase+i];

    sub_i2c_write(sh_, I2C_SADDRESS_SPIBRIDGE, 0, 0, cmd, 1+step);

  }

  return 0;

}

int uHTRClockMezzInterface::readSiLabsEEPROM(int which, std::vector<uint8_t>& image) {
  image.clear();

  char cmd[128];

  for (int ibase=0; ibase<512; ibase+=64) {
    cmd[0]=SPI_ADDR_EEPROM;
    cmd[1]=EEPROM_READ;
    cmd[2]=(which)?(0x80):(0)+((ibase>>8)&0xFF);
    cmd[3]=ibase&0xFF;
    sub_i2c_write(sh_, I2C_SADDRESS_SPIBRIDGE, 0, 0, cmd, 1+3+64);
    sub_i2c_read(sh_, I2C_SADDRESS_SPIBRIDGE, 0, 0, cmd, 3+64);

    for (int i=0; i<64; i++) {
      image.push_back(cmd[i+3]);
    }
  }

  return 0;  

}

float uHTRClockMezzInterface::readClockFreq(int which) {

  uint8_t csr=getCSR();
  csr|=CSR_START_MEASURE;
  setCSR(csr);

  csr=csr^CSR_START_MEASURE;
  if (which) csr|=CSR_WHICH_MEASURE;
  else csr&=(0xFF^CSR_WHICH_MEASURE);

  setCSR(csr);

  usleep(1000);

  uint8_t lowW,highW;


  for (int i=0; i<8; i++) {
    lowW=regRead(2);
    highW=regRead(3);
  }

  return (((uint32_t)highW)*256+lowW)*1000;
}

int uHTRClockMezzInterface::testBoardSetResets(bool siA, bool siB) {
  if(!openSuccessful_) return 999;

  char cmd[128];

  int step=2;

  cmd[0]=GPIO_DIRECTION;
  cmd[1]=GPIO_RESET_5319A|GPIO_RESET_5319B;

  sub_i2c_write(sh_, I2C_SADDRESS_U4_IO, 0, 0, cmd, 2);

  cmd[0]=GPIO_WRITE;
  cmd[1]=0;
  if (!siA) cmd[1]|=GPIO_RESET_5319A;
  if (!siB) cmd[1]|=GPIO_RESET_5319B;

  sub_i2c_write(sh_, I2C_SADDRESS_U4_IO, 0, 0, cmd, 2);

  cmd[0]=GPIO_READ;
  cmd[1]=0;
  sub_i2c_write(sh_, I2C_SADDRESS_U4_IO, 0, 0, cmd, 2);
  sub_i2c_read(sh_, I2C_SADDRESS_U4_IO, 0, 0, cmd, 2);

  printf("%x %x\n", cmd[0]&0xFF,cmd[1]&0xFF);

}
