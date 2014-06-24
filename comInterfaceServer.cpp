#include "comInterfaceServer.h"
#include <ctime>


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

void printCom(int err, int sa,  char buf[], int sz, char type, int i )
{

    FILE * out = stdout;
    if(err) 
    {
        time_t rawtime;
        struct tm * timeinfo;
        char buffer [80];
        time (&rawtime);
        timeinfo = localtime (&rawtime);

        strftime(buffer,80,"%F %X ",timeinfo);
        out = stderr;
        fprintf(out, buffer);
    }
    fprintf(out,"%c: %02x i: %d err:%d val:",type,sa,i,err);
    for(int j =0; j < sz; j++)
    {
        if(err) buf[j] = 0x00;
        fprintf(out,"%02x ", buf[j]);
    }
    fprintf(out,"\n");

    fflush(out);
}

int RPiInterfaceServer::i2c_write(int sa, char * buf, int sz, bool isMUX)
{
#ifdef URPI
    int i = 0;
    bool match = true ;
    do
    {
        i2c.setAddress(sa);
        i2c.send((unsigned char *)buf, sz);
        errno_ = i2c.fail();
        if(isMUX)
        {
            unsigned char *tmp = new unsigned char[sz];
            usleep(1000);

            i2c.receive((unsigned char *)tmp, sz);
            errno_ |= i2c.fail();

            for(int j = 0; j < sz; ++j) match &= (buf[j] == tmp[j]);
            
            delete [] tmp;
        }
    } while ((errno_ || (isMUX && !match)) && ++i < 19 && !usleep((i < 10)?1000:10000));

    printCom(errno_,sa, buf,sz,'w', i);

    return errno_;
#else
    return 0;
#endif
}

int RPiInterfaceServer::i2c_read(int sa, char * buf, int sz)
{
#ifdef URPI
    int i = 0;
    do
    {
        i2c.setAddress(sa);
        i2c.receive((unsigned char *)buf, sz);

        errno_ = i2c.fail();
    } while (errno_ && ++i < 19 && !usleep((i < 10)?1000:10000));

    printCom(errno_,sa, buf,sz,'r', i);

    return errno_;
#else
    return 0;
#endif
}

void RPiInterfaceServer::startTest(int pid, int adChan)
{
    printf("Starting channel %d with pid %d\n",adChan, pid);
    pids[adChan] = pid;
    time_t rawtime;
    time(&rawtime);
    times[adChan] = rawtime;
}

void RPiInterfaceServer::stopTest(int adChan)
{
    std::map<int,time_t>::iterator it = times.find(adChan);
    if(it != times.end())
    {
        int pid = pids[adChan];
        printf("Stopping channel %d with pid %d\n",adChan, pid);
        pids.erase(adChan);
        times.erase(adChan);
    }
    else
    {
        printf("No test on channel %d\n",adChan);
    }
}

void RPiInterfaceServer::readTest(int adChan, int data[])
{
    std::map<int,time_t>::iterator it = times.find(adChan);
    if(it != times.end())
    {
        time_t rawtime;
        time(&rawtime);
        data[0] = rawtime - times[adChan];
        data[1] = pids[adChan];
    }
    else 
    {
        data[0] = 0;
        data[1] = 0;
    }
    printf("Channel %d: Time %d, pid: %d", adChan, data[0], data[1]);
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
