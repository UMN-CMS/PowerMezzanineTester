class ComInterfaceServer
{
public:
    ComInterfaceServer() {};
    virtual int i2c_write(int sa, char * buf, int sz) = 0;
    virtual int i2c_read(int sa, char * buf, int sz) = 0;
    virtual void lcd_write(char *,int) = 0;
    int getError()
    {
	return errno_;
    }
protected:
    int errno_;    
};


#ifdef URPI

#define BOARD RASPBERRY_PI
#include "gnublin-api/gnublin.h"

#endif

class RPiInterfaceServer : public ComInterfaceServer
{
public:
    RPiInterfaceServer();
    int i2c_write(int sa, char * buf, int sz);
    int i2c_read(int sa, char * buf, int sz);
    void lcd_write(char *, int sz);

private:
#ifdef URPI
    gnublin_i2c i2c;
#else
    int i2c;
#endif
};


