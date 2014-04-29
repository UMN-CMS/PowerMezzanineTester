#ifndef UHTR_MOD_INTERFACE
#define UHTR_MOD_INTERFACE

#include <stdint.h>
#include "comInterface.h"
#include <string>

enum MUXChannels
{
    MUX_POWERMOD = 0,
    MUX_AUXPOWERMOD = 1
} ;

enum V2MUXChannels
{
    V2_MUX_PM_3_3  = 1,
    V2_MUX_PM_1_A  = 5,
    V2_MUX_PM_1_B  = 6,
    V2_MUX_APM_2_5 = 2,
    V2_MUX_APM_1_6 = 4    
} ;

class uHTRMezzInterface
{
public:
    const bool isV2_;
    const bool isRPi_;
    
    uHTRMezzInterface(int idev = 0, bool v2 = true, bool isRPi = true, const char host[] = "localhost", const char port[]= "1338");
    virtual ~uHTRMezzInterface();
    int setMUXChannel(const int, const int);
    virtual int readMezzMAC();
    void printMezzMAC();
    void copyMezzMAC(char MAC[6]);
    
    void updateSUB20Display(const char *);
    void togglePowerMezzs(const bool); 

    bool can_connect();
    void init();
    void startTest(int pid);
    void stopTest();
    void readTest(int test[]);
    
    inline unsigned int getError()
    {
	return errval_;
    }
    
protected:
    ComInterface *com;
    unsigned int errval_;
    bool openSuccessful_;
    unsigned char buff_[128], MAC_[6];
};

#endif
