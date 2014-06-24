class ComInterfaceServer
{
public:
    ComInterfaceServer() {};
    virtual int i2c_write(int sa, char * buf, int sz, bool isMUX) = 0;
    virtual int i2c_read(int sa, char * buf, int sz) = 0;
    virtual void lcd_write(char *,int) = 0;
    int getError()
    {
	return errno_;
    }
protected:
    int errno_;    
    virtual void configADC128() {} 
};


#ifdef URPI

#define BOARD RASPBERRY_PI
#include "gnublin-api/gnublin.h"
#include <time.h>
#include <map>

#endif

class RPiInterfaceServer : public ComInterfaceServer
{
public:
    RPiInterfaceServer();
    int i2c_write(int sa, char * buf, int sz, bool isMUX = false);
    int i2c_read(int sa, char * buf, int sz);
    void lcd_write(char *, int sz);
    void startTest(int pid, int adChan);
    void stopTest(int adChan);
    void readTest(int adChan, int data[]);

private:
    void configADC128();
    std::map<int,time_t> times;
    std::map<int,int> pids;
#ifdef URPI
    gnublin_i2c i2c;
#else
    int i2c;
#endif
};


