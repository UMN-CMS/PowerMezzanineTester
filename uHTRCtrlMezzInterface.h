#ifndef UHTRCTRLMEZZINTERFACE_H
#define UHTRCTRLMEZZINTERFACE_H

#include "libsub.h"
#include <readline/readline.h>
#include <readline/history.h>
//#include "uHTRMezzInterface.h"
//#include <vector>
//#include <string>


class uHTRCtrlMezzInterface
{
public:

  uHTRCtrlMezzInterface() { } ;
  ~uHTRCtrlMezzInterface() { } ;
  
  void set_sub20_frequence( sub_handle& hd_ ) ;
  void scan_i2c_slaves( sub_handle& hd_ ) ;
  void Test( sub_handle& hd_ ) ;
  void TestMMC( sub_handle& hd_ ) ;
  void spi_test( sub_handle& hd_ ) ;

  // General SPI functions
  void spi_switch( sub_handle& hd_, int channel ) ;
  void spi_config( sub_handle& hd_ ) ;
  void spi_sector_erase( sub_handle& hd_, int addr ) ;
  void spi_read( sub_handle& hd_, std::vector<char> instruct, std::vector<char>& output, int readsize ) ;
  void spi_write( sub_handle& hd_, std::vector<char> address, std::vector<unsigned char>& input, int writesize, int input_i = 0 ) ;
  bool spi_status( sub_handle& hd_, int bitPos ) ;

  // For CPLD Mezzanine
  void readEEPROM( sub_handle& hd_ ) ;
  void checkEEPROMRegister( sub_handle& hd_ ) ;

  // For Flash Mezzanine
  void checkFlashID( sub_handle& hd_ ) ;
  void writeFirmware( sub_handle& hd_ ) ;
  void readFirmwareSector( sub_handle& hd_, std::vector<unsigned char>& inFlash, int iSector, int iPage = -1 ) ;
  bool verifyFirmware( sub_handle& hd_ ) ;
  void readMCSFile( std::vector<std::string>& lines ) ;
  void readMCSFile( std::vector<unsigned char>& source ) ;
  void printMCSFile() ;

  unsigned char Char2Hex(unsigned char c) ;
  std::string tool_readline( std::string prompt ) ;

//private:


};

#endif
