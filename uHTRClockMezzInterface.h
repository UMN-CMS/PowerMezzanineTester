#ifndef UHTR_CLOCKMOD_INTERFACE
#define UHTR_CLOCKMOD_INTERFACE

#include "uHTRMezzInterface.h"
#include <vector>

class uHTRClockMezzInterface : public uHTRMezzInterface 
{
public:
  uHTRClockMezzInterface(int);
  virtual int readMezzMAC();
  int setCSR(uint8_t value);
  int getCSR();
  int getFWVersion();
  
  int programSiLabsEEPROM(int which, const char* fname);
  int programSiLabsDirect(const char* fname);
  int readSiLabsEEPROM(int which, std::vector<uint8_t>&);  
  float readClockFreq(int which);

  int testBoardSetResets(bool siA, bool siB);
private:
  uint8_t regRead(int addr);
  void regWrite(int addr,uint8_t val);
};

#endif
