#include "uHTRMezzInterface.h"
#include <cstdio>
#include "io.h"

// i2c slave addresses
#define I2C_SADDRESS_BASEMUX   0x70
#define I2C_SADDRESS_EEPROM    0x50
#define I2C_SADDRESS_MODADC_26 0x68
#define I2C_SADDRESS_MODADC_28 0x6A
#define I2C_SADDRESS_MARGCTRL  0x20

#define V2_I2C_SADDRESS_BASE_MUX    0x70
#define V2_I2C_SADDRESS_PMBASE_ADC  0x6e
#define V2_I2C_SADDRESS_PMBASE_GPIO 0x3f
#define V2_I2C_SADDRESS_APMBASE_ADC  0x1d
#define V2_I2C_SADDRESS_APMBASE_GPIO 0x3f

#define V2_I2C_SADDRESS_RPI_MUX    0x77

// i2c adc values
#define I2C_MODADC_WRITE_DEFAULT 0x90
#define I2C_MODADC_WRITE_CHAN1   0x00
#define I2C_MODADC_WRITE_CHAN2   0x20
#define I2C_MODADC_WRITE_CHAN3   0x40
#define I2C_MODADC_WRITE_CHAN4   0x60

#define I2C_MODADC_READMASK_RDY       0x80
#define I2C_MODADC_READMASK_CHAN      0x60
#define I2C_MODADC_READMASK_RATE      0x0B
#define I2C_MODADC_READMASK_GAIN      0x03
#define I2C_MODADC_READMASK_UPBYTE_12 0x0f

//SUB-20 gpio channels
#define S20_GPIO_C12 0x01000
#define S20_GPIO_C16 0x10000
#define S20_GPIO_C17 0x20000
#define S20_GPIO_C18 0x40000
#define S20_GPIO_C19 0x80000

uHTRMezzInterface::uHTRMezzInterface(int idev, bool v2, bool isRPi, const char host[], const char port[]) : isV2_(v2), isRPi_(isRPi), openSuccessful_(false)
{
    //open communication interface
    com = 0;
    if(isRPi)
    {
	com = new RPiInterface(host,port);
    }
    else 
    {
	com = new S20Interface();
    }

    if(com) 
    {
	openSuccessful_ = true;
	errval_ = 0;
    }
    else
    {
	openSuccessful_ = false;
        errval_ = 999;
    }
}

uHTRMezzInterface::~uHTRMezzInterface(){ }

int uHTRMezzInterface::setMUXChannel(const int bbChan, const int adChan)
{
    if(!openSuccessful_) return 999;

    int error = 0;
    

    if(isRPi_ && adChan >= 0)
    {
        com->set_adChan(adChan);
    }

    if(isRPi_)
    {
        com->set_bbChan(bbChan);
    }
    else
    {

        if(isV2_)
        {
            buff_[0] = (char)(1 << bbChan);
            error |= com->i2c_write(V2_I2C_SADDRESS_BASE_MUX, (char*)buff_, 1);
        }
        else
        {
            buff_[0] = (char)(bbChan | 0x4);
            error |= com->i2c_write(I2C_SADDRESS_BASEMUX, (char*)buff_, 1);
        }
    }

    return error;
}

int uHTRMezzInterface::readMezzMAC()
{
    if(!openSuccessful_) return 999;

    MAC_[0] = MAC_[1] = MAC_[2] = MAC_[3] = MAC_[4] = MAC_[5] = 0x00;

    int error = 0;

    buff_[0] = 0xfa; //address to start of MAC
    error = com->i2c_write(I2C_SADDRESS_EEPROM, (char*)buff_, 1);
    error |= com->i2c_read(I2C_SADDRESS_EEPROM, (char*)MAC_, 6); //MAC is 6 bytes long

    errval_ = error;

    //io::printf("eprom read retval: %d buffval: %x %x %x %x %x %x\n", errval_, MAC_[0]&0xFF,MAC_[1]&0xFF,MAC_[2]&0xFF,MAC_[3]&0xFF,MAC_[4]&0xFF,MAC_[5]&0xFF);

    return !(MAC_[0] || MAC_[1] || MAC_[2] || MAC_[3] || MAC_[4] || MAC_[5]);
}

void uHTRMezzInterface::printMezzMAC()
{
    //if(isRPi_)
    //{
	//com->i2c_read(V2_I2C_SADDRESS_RPI_MUX, (char*)buff_, 1);
	//int i = 0;
	//for(; i < 8; i++) if((buff_[0] >> i)&0x1) break;
    //io::printf("RPi adapter channel: %d\n", i);
    //}

    io::printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", MAC_[0]&0xFF, MAC_[1]&0xFF, MAC_[2]&0xFF, MAC_[3]&0xFF, MAC_[4]&0xFF, MAC_[5]&0xFF);
}

void uHTRMezzInterface::copyMezzMAC(char * MAC)
{
    for(int i = 0; i < 6; i++) MAC[i] = MAC_[i];
}

void uHTRMezzInterface::updateSUB20Display(const char * dbuf)
{
    if(!openSuccessful_) return;

    com->lcd_write((char*)dbuf,strlen(dbuf));

    return;
}

void uHTRMezzInterface::init()
{
}

bool uHTRMezzInterface::can_connect()
{
    return com->can_connect();
}

void uHTRMezzInterface::startTest(int pid)
{
    com->startTest(pid);
}

void uHTRMezzInterface::stopTest()
{
    com->stopTest();
}

void uHTRMezzInterface::readTest(int test[])
{
    com->readTest(test);
}
