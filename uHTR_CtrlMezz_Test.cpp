#include <cstdlib>
#include <cstdio>
#include <unistd.h> 
#include <getopt.h>
#include <time.h>
#include <map>
#include <string>
#include <vector>
//#include "uHTR_CtrlMezz_Test.h"
#include "uHTRCtrlMezzInterface.h"
#include <readline/readline.h>
#include <readline/history.h>
//#include <boost/thread.hpp>
//#include <boost/bind.hpp>

using namespace std ;

int spi_channel = 2 ;

void help() ;
bool InitializeSub20( sub_device& dev_ , sub_handle& hd_ ) ;

void help()
{
    printf("Usage: ./uHTR_CtrlMezz_Test.exe modeFlags [options]\n");
    printf("Mode Flags - These can be used in conjunction but will always evaluate in the order listed below.\n");
    printf(" --initial,         -i   Initialize sub20\n");
    printf(" --scan,            -s   Scan i2c slaves\n");
    printf(" --cpld,            -c   Test CPLD\n");
    printf(" --readflash,       -r   Read Flash\n");
    printf(" --writeflash,      -w   Write Flash\n");
    printf(" --printMCS,        -p   Print MCS file\n");
    printf(" --mmc,             -t   Test MMC\n");
    printf(" --help,            -h   Showing this help menu\n");
}

int main(int argc, char* argv[])
{

    int opt;
    int option_index = 0;
    static struct option long_options[] = {
        {"initial",             no_argument, 0, 'i'},
        {"scan",                no_argument, 0, 's'},
        {"mmc",                 no_argument, 0, 't'},
        {"cpld",          required_argument, 0, 'c'},
        {"readflash",     required_argument, 0, 'r'},
        {"writeflash",    required_argument, 0, 'w'},
        {"printMCS",            no_argument, 0, 'p'},
        {"help",                no_argument, 0, 'h'}
    };

    if(argc == 1) {
        help();
        return 0;
    }

    uHTRCtrlMezzInterface* ctrl = new uHTRCtrlMezzInterface() ;
    sub_device dev = 0 ;
    sub_handle hdl = 0 ;

    while((opt = getopt_long(argc, argv, "ipsth:c:w:r:", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 't':
                InitializeSub20( dev, hdl );
                ctrl->set_sub20_frequence( hdl ) ;
                ctrl->TestMMC( hdl ) ;          
                break;
            case 'i':
                InitializeSub20( dev, hdl );
                break;
            case 'c':
                spi_channel = int(atoi(optarg));

                InitializeSub20( dev, hdl );
                ctrl->set_sub20_frequence( hdl ) ;
                ctrl->spi_switch( hdl, spi_channel ) ;          
                ctrl->readEEPROM( hdl ) ;
                break;
            case 'w':
                spi_channel = int(atoi(optarg));

                InitializeSub20( dev, hdl );
                ctrl->set_sub20_frequence( hdl ) ;
                ctrl->spi_switch( hdl , spi_channel  ) ;          
                ctrl->spi_config( hdl) ;
                ctrl->writeFirmware( hdl ) ;
                break;
            case 'r':
                spi_channel = int(atoi(optarg));

                InitializeSub20( dev, hdl );
                ctrl->set_sub20_frequence( hdl ) ;
                ctrl->spi_switch( hdl , spi_channel ) ;          
                ctrl->spi_config( hdl) ;
                ctrl->verifyFirmware( hdl ) ;
                break;
            case 'p':
                ctrl->printMCSFile() ;
                break;
            case 's':
                InitializeSub20( dev, hdl );
                ctrl->scan_i2c_slaves( hdl ) ;
                break;
            case 'h':
                help() ;
                break ;
            default:
                help() ;
                return 0;

        }
    }


   return 0 ;

}

bool InitializeSub20( sub_device& dev_ , sub_handle& hd_ ) {

    bool found_s20 = false ;
    int k = 0 ;
    while ( !found_s20 && k < 10 ) {

           dev_ = sub_find_devices( dev_ ) ;
  
            if ( dev_ !=0 ) {
        
               printf(" found dev %d \n", k ) ; 
	       hd_ = sub_open( dev_ );
	       if ( hd_ == 0) { 
                  printf("DEVICE FAILED TO OPEN\n");
                  unsigned int errval_ = sub_errno;
		  unsigned int i2c_status_ = sub_i2c_status;
		  printf(" Dev(%d) error : %u  i2c_stat: %u \n\n", k, errval_ , i2c_status_ ) ;
               } 
	       else {
                  found_s20 = true ;
               }
            }

            k++ ;
    }

    // check the sub20 sn 
    char buf[100] ;
    int sn = sub_get_serial_number( hd_ , buf, sizeof( buf ) ) ;
    printf(" sn: %d = %s \n", sn, buf );
 
    return found_s20 ;

}

