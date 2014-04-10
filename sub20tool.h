#ifndef SUB20TOOL_H
#define SUB20TOOL_H

#include <vector>
#include "comInterface.h"

using namespace std ;

class sub20tool
{
public:

  sub20tool() 
   {
     i2c = new RPiInterface();
   } 
  ~sub20tool() { }
  
  bool InitializeSub20( ) ;
  void set_sub20_frequence(int freq = 2000 ) ;
  void scan_i2c_slaves();
  void action(vector<string>& cmdv ) ;
  void RunCmdFile(string theFile ) ;

  void read(int addr, int nbyte = 1 ) ;
  void write(vector<string>& cmdv ) ;
  void sub20_help() ;

private:
  ComInterface *i2c;

};

#endif
