class ComInterface
{
public:
    ComInterface() {};
    virtual int i2c_write(int sa, char * buf, int sz) = 0;
    virtual int i2c_read(int sa, char * buf, int sz) = 0;
    virtual double read_adc(int chan) = 0;
    virtual void lcd_write(char *,int) = 0;
    virtual void set_adChan(int adChan) {}

    virtual bool can_connect() {return false;}
    virtual void configADC128() {} 

    virtual bool isRPi() = 0;
    int getError()
    {
	return errno_;
    }
protected:
    int errno_;    
};

#ifdef USUB20

#include "libsub.h"

#endif

class S20Interface : public ComInterface
{
public:
    S20Interface(int ns20 = 0);
    int i2c_write(int sa, char * buf, int sz);
    int i2c_read(int sa, char * buf, int sz);
    double read_adc(int chan);
    void lcd_write(char *,int sz);
    bool isRPi() {return false;}

#ifdef USUB20
private:
    sub_handle sh_;
    bool openSuccessful_;
#endif
};


#ifdef URPI

#define BOARD RASPBERRY_PI
#include "gnublin-api/gnublin.h"
#include <boost/asio.hpp>

#endif

enum Mode
{
    READ = 1,
    WRITE = 2,
    DISPLAY = 3
};

class RPiInterface : public ComInterface
{
public:
    RPiInterface(std::string host, std::string port);
    int i2c_write(int sa, char * buf, int sz);
    int i2c_read(int sa, char * buf, int sz);
    double read_adc(int chan);
    void lcd_write(char *,int sz); //RPi has no LCD display so write to std::cout
    void set_adChan(int adChan) { adChan_ = adChan; }
    bool isRPi() {return true;}

    bool open_socket();
    bool can_connect();

    void configADC128();

private:
    void send_header(int address, Mode mode, int sz);
    int recieve_error();
    static boost::asio::io_service * io_service;
    boost::asio::ip::tcp::resolver::iterator iterator;
    boost::asio::ip::tcp::socket * s;
    int adChan_;
};


