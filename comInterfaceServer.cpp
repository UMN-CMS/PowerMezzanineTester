#include "comInterfaceServer.h"


#define RPI_MUX_SADDRESS 0x77
#define RPI_ADC_SADDRESS 0x1e

RPiInterfaceServer::RPiInterfaceServer() : i2c()
{

}

void RPiInterfaceServer::lcd_write(char * buf, int sz)
{
#ifdef URPI
    std::cout << "LCD_WRITE: " << std::string(buf,sz) << std::endl;
#endif
}

int RPiInterfaceServer::i2c_write(int sa, char * buf, int sz)
{
#ifdef URPI
    i2c.setAddress(sa);
    i2c.send((unsigned char *)buf, sz);

    errno_ = i2c.fail();
    
    return i2c.fail();
#else
    return 0;
#endif
}

int RPiInterfaceServer::i2c_read(int sa, char * buf, int sz)
{
#ifdef URPI
    i2c.setAddress(sa);
    i2c.receive((unsigned char *)buf, sz);
    
    errno_ = i2c.fail();
    
    return i2c.fail();
#else
    return 0;
#endif
}


