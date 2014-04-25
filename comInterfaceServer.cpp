#include "comInterfaceServer.h"


#define RPI_MUX_SADDRESS 0x77
#define RPI_ADC_SADDRESS 0x1d

RPiInterfaceServer::RPiInterfaceServer() : i2c()
{
    configADC128();
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

    printf("w: %02x: %d: ", sa,errno_);
    for(int i =0; i < sz; i++)
        printf("%02x ", buf[i]);
    printf("\n");
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

    printf("r: %02x: %d: ", sa,errno_);
    for(int i =0; i < sz; i++)
        printf("%02x ", buf[i]);
    printf("\n");

    return i2c.fail();
#else
    return 0;
#endif
}

void RPiInterfaceServer::startTest(int pid, int adChan)
{
    pids[adChan] = pid;
    time_t rawtime;
    time(&rawtime);
    times[adChan] = rawtime;
}

void RPiInterfaceServer::stopTest(int adChan)
{
    pids.erase(adChan);
    times.erase(adChan);
}

void RPiInterfaceServer::readTest(int adChan, int data[])
{
    std::map<int,time_t>::iterator it = times.find(adChan);
    if(it != times.end())
    {
        data[0] = times[adChan];
        data[1] = pids[adChan];
    }
    else 
    {
        data[0] = -1;
        data[1] = -1;
    }
}


void RPiInterfaceServer::configADC128()
{
#ifdef URPI
    unsigned int error = 0;

    char buff[9];

    // ensure mux points to the right address
    buff[0] = 0x01;
    error |= i2c_write(RPI_MUX_SADDRESS, buff, 1);

    // Wait until chip is ready
    buff[0] = 0x0c; //location of busy status register
    error |= i2c_write(RPI_ADC_SADDRESS, (char*)buff, 1);
    do
    {
        error |= i2c_read(RPI_ADC_SADDRESS, (char*)buff, 1);
    } while(!error && (buff[0] & 0x02) && !usleep(100000));

    // make sure adc is off
    buff[0] = 0x00; //location of config register
    buff[1] = 0x80; //disable and reset
    error |= i2c_write(RPI_ADC_SADDRESS, (char*)buff, 2);
    
    // Set conversion rate register 
    buff[0] = 0x07; //location of conversion rate register
    buff[1] = 0x01; //continous conversion rate 
    error |= i2c_write(RPI_ADC_SADDRESS, (char*)buff, 2);

    // Mask unused channels
    buff[0] = 0x08; //location of mask register
    buff[1] = 0x00; //mask no channels
    error |= i2c_write(RPI_ADC_SADDRESS, (char*)buff, 2);

    // Advanced config
    buff[0] = 0x0b; //location of advanced config register
    buff[1] = 0x02; //set internal ref and mode 1 for all 8 ADC in 
    error |= i2c_write(RPI_ADC_SADDRESS, (char*)buff, 2);
    
    // Start adc
    buff[0] = 0x00; //location of config register
    buff[1] = 0x01; //enable
    error |= i2c_write(RPI_ADC_SADDRESS, (char*)buff, 2);

    usleep(100000);

    errno_ = error;
#endif
}
