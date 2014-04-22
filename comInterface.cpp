#include "comInterface.h"

S20Interface::S20Interface(int ns20)
{
#ifdef USUB20

    int count = 0;
    sub_device sd;
    for (sd = sub_find_devices(sd_); sd != 0; sd = sub_find_devices(sd_))
    {
	if(count == ns20)
        {
	    sh_ = sub_open(sd);
	    break;
        }
	count++;
    }
    
    //verify the device is open                                                                                                                              
    if(sh_ == 0) 
    {
	printf("DEVICE FAILED TO OPEN\n");
	openSuccessful_ = false;
    }
    else
    {
      openSuccessful_ = true;

      //configure i2c settingns                                                                                                                            
      int freq = 100000;

      sub_i2c_freq(sh_, &freq);

      //configure adc sattings                                                                                                                             
      sub_adc_config(sh_, ADC_ENABLE | ADC_REF_2_56);

    }                                                                                         
    errno_ = sub_i2c_status;
#else
    std::cerr << "SUB20 NOT INSTALLED!\n";
#endif
}

int S20Interface::i2c_write(int sa, char * buf, int sz)
{
#ifdef USUB20

    if(!openSuccessful_) return 999;
    
    sub_i2c_write(sh_, sa, 0, 0, buf, sz);

    errno_ = sub_i2c_status;

    return sub_i2c_status;
#else
    return 0;
#endif
}

int S20Interface::i2c_read(int sa, char * buf, int sz)
{
#ifdef USUB20

    if(!openSuccessful_) return 999;
    
    sub_i2c_read(sh_, sa, 0, 0, buf, sz);
    
    errno_ = sub_i2c_status;
    
    return sub_i2c_status;
#else 
    return 0;
#endif
}

void S20Interface::lcd_write(char * buf, int sz)
{
#ifdef USUB20
    if(!openSuccessful_) return 999;

    sub_lcd_write(sh_, buf);
#endif
}

double S20Interface::read_adc(int chan)
{
#ifdef USUB20

    if(!openSuccessful_) return -999;

    switch(chan)
    {
        case 0:
            chan = ADC_S0;
            break;
        case 1:
            chan = ADC_S1;
            break;
        case 2:
            chan = ADC_S2;
            break;
        case 3:
            chan = ADC_S3;
            break;
        case 4:
            chan = ADC_S4;
            break;
        case 5:
            chan = ADC_S5;
            break;
        case 6:
            chan = ADC_S6;
            break;
        case 7:
            chan = ADC_S7;
            break;
    }

    int adcval;
    sub_adc_single(sh_, &adcval, chan);

    errval_ = sub_errno;

    return adcval * 2.56 / 1023;
#else
    return 0;
#endif
}

#define RPI_MUX_SADDRESS 0x77
#define RPI_ADC_SADDRESS 0x1d

using boost::asio::ip::tcp;

RPiInterface::RPiInterface(std::string host, std::string port) 
{
#ifdef URPI
    io_service = new boost::asio::io_service();

    tcp::resolver resolver(*io_service);
    tcp::resolver::query query(tcp::v4(), host, port);
    iterator = resolver.resolve(query);

    configADC128();
#else
    std::cerr << "RPI NOT INSTALLED!\n";
#endif
}

int RPiInterface::i2c_write(int sa, char * buf, int sz)
{
#ifdef URPI
    open_socket();
    send_header(sa,WRITE,sz);

    boost::asio::write(*s, boost::asio::buffer(buf, sz));

    errno_ = recieve_error();
    s->close();
    delete s;
    return errno_;
#else 
    return 0;
#endif

}

int RPiInterface::i2c_read(int sa, char * buf, int sz)
{
#ifdef URPI
    open_socket();
    send_header(sa,READ,sz);

    boost::asio::read(*s, boost::asio::buffer(buf,sz));
    errno_ = recieve_error();

    s->close();
    delete s;
    return errno_;
#else 
    return 0;
#endif
}

void RPiInterface::lcd_write(char * buf, int sz)
{
#ifdef URPI
    sz--;
    buf++;
    open_socket();
    send_header(0,DISPLAY,sz);

    boost::asio::write(*s, boost::asio::buffer(buf, sz));

    int ret = recieve_error();
    s->close();
    delete s;
#endif
}

void RPiInterface::open_socket()
{
#ifdef URPI
    s =  new tcp::socket(*io_service);
    s->connect(*iterator);
#endif
}

void RPiInterface::send_header(int address, Mode mode, int length)
{
#ifdef URPI
    std::vector<int> header(4);
    header[0]=address;
    header[1]=mode;
    header[2]=length;
    header[3]=adChan_;
    boost::asio::write(*s, boost::asio::buffer(header, 4*sizeof(int)));
#endif
}
        
int RPiInterface::recieve_error()
{
#ifdef URPI
    int error = 0;
    boost::asio::read(*s, boost::asio::buffer(&error,sizeof(error)));
    return error;
#else 
    return 0;
#endif
}

double RPiInterface::read_adc(int chan)
{
#ifdef URPI
    unsigned char buff[9], pLoc[2];

    int error = 0;

    //Set adapter MUX
    int tmp_adChan = adChan_;
    set_adChan(0);

    //Read ADC value
    if(chan >= 8) printf("INVALID ADC CHANNEL (ADC128, adapter)\n");
    // 0x20 is start of output registers 
    buff[0] = 0x20 + chan;
    error |= i2c_write(RPI_ADC_SADDRESS, (char*)buff, 1);  //set result register
    error |= i2c_read(RPI_ADC_SADDRESS, (char*)buff, 2);  //read result register (2 bytes for 12 bit vaule)

    //Write origional mux settings
    set_adChan(tmp_adChan);

    errno_ = error;
    unsigned int twelvebit=(unsigned int)(buff[0]);
    twelvebit=twelvebit<<4 | ((buff[1]&0xF0)>>4);
    double voltage=twelvebit/1600.0;

    return voltage;
#else
    return -999.0;
#endif
}

void RPiInterface::configADC128()
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
