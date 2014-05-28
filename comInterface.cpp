#include "comInterface.h"
#include "io.h"

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
        io::printf("DEVICE FAILED TO OPEN\n");
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

boost::asio::io_service * RPiInterface::io_service = NULL;

RPiInterface::RPiInterface(const std::string host, const std::string port) 
{
#ifdef URPI
    if(io_service == NULL)
    {
        io_service = new boost::asio::io_service();
    }
    adChan_ = -1;
    bbChan_ = -1;

    tcp::resolver resolver(*io_service);
    query = new tcp::resolver::query(tcp::v4(), host, port, boost::asio::ip::resolver_query_base::numeric_service);
    iterator = resolver.resolve(*query);

#else
    std::cerr << "RPI NOT INSTALLED!\n";
#endif
}

RPiInterface::~RPiInterface()
{
    delete[] io_service;
    delete[] query;
    delete[] s;
}


int RPiInterface::i2c_write(int sa, char * buf, int sz)
{
#ifdef URPI
    if(!open_socket()) return 1;

    send_header(sa,WRITE,sz);
    try
    {
        boost::asio::write(*s, boost::asio::buffer(buf, sz));
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception in i2c_write: " << e.what() << "\n";
    }

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
    if(!open_socket()) return 1;
    send_header(sa,READ,sz);

    try
    {
        boost::asio::read(*s, boost::asio::buffer(buf,sz));
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception in i2c_read: " << e.what() << "\n";
    }
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
    if(!open_socket()) return;
    send_header(0,DISPLAY,sz);

    try
    {
        boost::asio::write(*s, boost::asio::buffer(buf, sz));
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception in lcd_write: " << e.what() << "\n";
    }

    errno_ = recieve_error();

    s->close();
    delete s;
#endif
}

bool RPiInterface::can_connect()
{
#ifdef URPI
    bool can_open = open_socket();

    s->close();

    return can_open;
#endif
    return false;
}

bool RPiInterface::open_socket()
{
#ifdef URPI
    s =  new tcp::socket(*io_service);
    boost::system::error_code error = boost::asio::error::host_not_found;

    tcp::resolver::iterator end;
    if(iterator == end)
    {
        tcp::resolver resolver(*io_service);
        iterator = resolver.resolve(*query);
    }

    s->connect(*iterator,error);

    while (error && iterator != end)
    {
        s->close();
        s->connect(*iterator++, error);
    }

    return !error;
#endif
    return false;
}

void RPiInterface::send_header(int address, Mode mode, int length)
{
#ifdef URPI
    std::vector<int> header(5);
    header[0]=address;
    header[1]=mode;
    header[2]=length;
    header[3]=adChan_;
    header[4]=bbChan_;
    try{
        boost::asio::write(*s, boost::asio::buffer(header, 5*sizeof(int)));
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception in send_header: " << e.what() << "\n";
    }
#endif
}

int RPiInterface::recieve_error()
{
#ifdef URPI
    int error = 0;
    //Error handling is hard
    //try
    //{
    //    boost::asio::read(*s, boost::asio::buffer(&error,sizeof(error)));
    //}
    //catch (std::exception& e)
    //{
    //    std::cerr << "Exception in recieve_error: " << e.what() << "\n";
    //}
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
    if(chan >= 8) io::printf("INVALID ADC CHANNEL (ADC128, adapter)\n");
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

void RPiInterface::startTest(int pid)
{
#ifdef URPI
    if(!open_socket()) return;
    send_header(pid,START,0);

    errno_ = recieve_error();

    s->close();
    delete s;
#endif
}

void RPiInterface::stopTest()
{
#ifdef URPI
    if(!open_socket()) return;
    send_header(0,STOP,0);

    errno_ = recieve_error();

    s->close();
    delete s;
#endif
}

void RPiInterface::readTest(int test[])
{
#ifdef URPI
    if(!open_socket()) return;
    send_header(0,TIME,2);

    test[0] = 0;
    test[1] = 0;
    try
    {
        boost::asio::read(*s, boost::asio::buffer(test,sizeof(int)*2));
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception in readTest: " << e.what() << "\n";
    }
    errno_ = recieve_error();

    s->close();
    delete s;
#endif
}
