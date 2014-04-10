#include <cstdlib>
#include <cstdio>
#include <unistd.h> 
#include <getopt.h>
#include <time.h>
#include <map>
#include <string>
#include <vector>
//#include <boost/thread.hpp>
//#include <boost/bind.hpp>
#include <readline/readline.h>
#include <readline/history.h>
#include "uHTRCtrlMezzInterface.h"

using namespace std ;

//uHTRCtrlMezzInterface::uHTRCtrlMezzInterface() { }
//uHTRCtrlMezzInterface::~uHTRCtrlMezzInterface() { }


/*
string tool_readline() ;
*/

string uHTRCtrlMezzInterface::tool_readline( string prompt ) {

    string retval;

    char* input = readline( prompt.c_str() );
    if ( input && *input ) add_history( input ) ;

    retval   = string( input );
    free( input );

    for ( size_t i = retval.size()-1 ; i >=0; i-- ) {
        if ( isspace( retval[i] ) )  retval.erase( i, 1 ) ;
        else break ;
    }
    //if (retval.empty()) retval=defval;
    return retval;
}

// set operation frequence of i2c communication for sub20
void uHTRCtrlMezzInterface::set_sub20_frequence( sub_handle& hd_ ) {

   if ( hd_ == 0 ) {
      printf(" Sub20 is not initialized, Please initialize sub20 first !!! " ) ;
      return ;
   }

   int freq = 2000 ;
   int f_ =   sub_i2c_freq( hd_ , &freq ) ;
   if ( f_ == 0 ) printf(" frequence is set to %d ! \n", freq ) ;
   else           printf(" freq set fail ! \n") ;

}

// In order to find the i2c slave , which is the top level mux in this test
void uHTRCtrlMezzInterface::scan_i2c_slaves( sub_handle& hd_ ) {

   if ( hd_ == 0 ) {
      printf(" Sub20 is not initialized, Please initialize sub20 first !!! " ) ;
      return ;
   }

   // target slave - i2c MUX
   int nSlave = 0 ;
   char sAddr[128];
   int s_ = sub_i2c_scan( hd_, &nSlave, sAddr ) ;
   printf(" ==> i2c(%d) scan : %d  \n", s_ , nSlave  ) ;

   for ( int j =0 ; j < nSlave ; j++ )
       printf("    -->addr: %x \n", sAddr[j] ) ;
}

void uHTRCtrlMezzInterface::TestMMC( sub_handle& hd_ ) {

   if ( hd_ == 0 ) {
      printf(" Sub20 is not initialized, Please initialize sub20 first !!! " ) ;
      return ;
   }

   // switch to different channel
   char wbuff[8] ;
   char rbuff[8] ;

   // switch to channel 1 - MMC 
   wbuff[0] = (char)(1 << 1);
   printf(" =========================== \n");
   printf(" Set to channel 1 - MMC \n" ) ;
   int w_ = sub_i2c_write( hd_, 0x70, 0, 0, wbuff, 1);
   if ( w_ != 0 )  printf(" switch to MMC failed !! \n") ;

   // MPC3426 ADC i2c address - 0x6f
   // 0x90 (10010000) : Initial new conversation in continuous 12-bit mode for channel 1
   wbuff[0] =  0x90 ;
   w_ = sub_i2c_write( hd_, 0x6F, 0, 0, wbuff, 1);
   if ( w_ != 0 )  printf(" configure error \n");
  
   for ( int k=0; k < 10 ; k++ ) {
   
   int r_ = sub_i2c_read( hd_, 0x6F, 0, 0, rbuff, 3);
   if ( r_ != 0 ) { 
      printf(" read error \n") ;
   }  else {
      double retval = double(int((0xf & rbuff[0]) << 8) | (int)rbuff[1]);
      printf(" read adc : [%x][%x][%x] = %.1f \n",  rbuff[2], rbuff[1], rbuff[0], retval  )  ;
   }  
  
   }

}

// CPLD at mux channel 3
void uHTRCtrlMezzInterface::spi_switch( sub_handle& hd_, int channel ) {

   if ( hd_ == 0 ) {
      printf(" Sub20 is not initialized, Please initialize sub20 first !!! " ) ;
      return ;
   }

   // switch to different channel
   char wbuff[8] ;
   char rbuff[8] ;

   // switch to i2c channle 4 - FLASH 
   // check whether FLASH is connected 
   wbuff[0] = (char)(1 << 4);
   printf(" =========================== \n") ;
   printf(" Set to mux ch4 - FlASH MEZZ \n" ) ;
   int w_ = sub_i2c_write( hd_, 0x70, 0, 0, wbuff, 1);
   if ( w_ != 0 )  printf(" switch to FLASH failed !! \n");
   scan_i2c_slaves( hd_ ) ;

   // switch to channle 3 - CPLD
   wbuff[0] = (char)(1 << 3);
   printf(" =========================== \n");
   printf(" Set to mux ch3 - CPLD \n" ) ;
   w_ = sub_i2c_write( hd_, 0x70, 0, 0, wbuff, 1);

   // GPIO [PCA9557] - Device address: 0x18
   // First read to check status

   // Disable polarity-inverse for all pins
   wbuff[0] = 0x2 ; 
   wbuff[1] = 0x0 ; 
   w_ = sub_i2c_write( hd_, 0x18, 0, 0, wbuff, 2);

   // Set pins to output
   wbuff[0] = 0x3 ; // set I/O as write 
   wbuff[1] = 0x0 ; // set I/O as write 
   w_ = sub_i2c_write( hd_, 0x18, 0, 0, wbuff, 2);

   // check status
   int r_ = sub_i2c_read( hd_, 0x18, 0, 0, rbuff, 3);
   if ( r_ != 0 ) printf(" read error \n");
   else           printf(" read value: [%x][%x][%x] \n",  rbuff[2], rbuff[1], rbuff[0] )  ;

   // 1st step, configure the pins for input/ouput  
   wbuff[0] = 0x1 ; // set I/O as write 
   //w_ = sub_i2c_write( hd_, 0x18, 0, 0, wbuff, 1);
   //if ( w_ != 0 )  cout<<" configur error 1" <<endl ;

   // SPI target select - 8 bits, bit 7~4 correspond to TargetSelect[3] ~ TargetSelect[0]
   // 0: BCK_CS0 , 1: BCK_CS1, 2: BCK_CS, 3: FNT_CS0 , 4: FNT_CS1, 5: FNT_CS, 
   // 8: CS_EEPROM , 9: CS_MAC, 10: CS_CPLD 
   char targetId = channel << 4  ;

   printf("\n ==== SPI TargetID %X ====\n", targetId ) ;
   wbuff[1] = targetId ; // FNT_CS
   w_ = sub_i2c_write( hd_, 0x18, 0, 0, wbuff, 2);
   if ( w_ != 0 )  printf(" configur error \n") ;
 

}


void uHTRCtrlMezzInterface::spi_test( sub_handle& hd_ ) {

   if ( hd_ == 0 ) {
      printf(" Sub20 is not initialized, Please initialize sub20 first !!! " ) ;
      return ;
   }

   /* Read current SPI configuration */
   printf(" ========================== \n") ;
   int spi_cfg = 0 ;

   /* Configure SPI */
   printf(" SPI_CFG = %d \n", SPI_ENABLE|SPI_CPOL_RISE|SPI_SMPL_SETUP|SPI_MSB_FIRST|SPI_CLK_500KHZ ) ;
   int rc = sub_spi_config( hd_, SPI_ENABLE|SPI_CPOL_RISE|SPI_SMPL_SETUP|SPI_MSB_FIRST|SPI_CLK_500KHZ, 0 ) ;
   sub_spi_config( hd_, 0, &spi_cfg );
   //int rc = sub_spi_config( hd_,  64, &spi_cfg ) ;
   if ( rc ==0 ) printf(" SPI configured (%d) \n", spi_cfg ) ;

   // Select spi targets - Always SS0 (because using DB9 connector )  
   char wbuff[128] ;
   char rbuff[256] ;

   // Read the Flash ID
   wbuff[0] = 0x9F ;
   rc = sub_spi_transfer( hd_, wbuff, 0, 1 , SS_CONF(0,SS_L) ) ;
   if ( rc !=0 ) printf(" SPI Write fail \n" ) ;

   rc = sub_spi_transfer( hd_, 0, rbuff, 3 , SS_CONF(0,SS_LO) ) ;
   if ( rc !=0 ) printf(" SPI READ fail - error code :%d \n", rc ) ;
   if ( rc ==0 ) { 
          printf(" Flash ID :        [%x] [%x] [%x] \n", rbuff[0], rbuff[1], rbuff[2]) ;
   }

   // Read status register
   wbuff[0] = 0x5 ;
   rc = sub_spi_transfer( hd_, wbuff, 0, 1 , SS_CONF(0,SS_L) ) ;
   if ( rc !=0 ) printf(" SPI Write fail \n" ) ;
   rc = sub_spi_transfer( hd_, 0, rbuff, 3 , SS_CONF(0,SS_LO) ) ;
   if ( rc !=0 ) printf(" SPI READ fail - error code :%d \n", rc ) ;
   if ( rc ==0 ) { 
          printf(" Status Register : [%x] [%x] [%x] \n\n", rbuff[0], rbuff[1], rbuff[2]) ;
   }

   // Read Flash Content
   int addr = 1*(0x40000) ;
   wbuff[0] = 0x3 ;
   wbuff[1] = (addr>>16)&0xFF ;
   wbuff[2] = (addr>>8)&0xFF ;
   wbuff[3] = addr&0xFF ;
   rc = sub_spi_transfer( hd_, wbuff, 0, 4 , SS_CONF(0,SS_L) ) ;
   if ( rc !=0 ) printf(" SPI Write fail \n" ) ;

   rc = sub_spi_transfer( hd_, 0, rbuff, 256 , SS_CONF(0,SS_LO) ) ;
   if ( rc !=0 ) printf(" SPI READ fail - error code :%d \n", rc ) ;
   if ( rc ==0 ) { 
      for ( int i=0 ; i < 16; i++ )
          printf(" SPI read : [%x] \n", rbuff[i] ) ;
   }

}


void uHTRCtrlMezzInterface::spi_config( sub_handle& hd_ ) {

   if ( hd_ == 0 ) {
      printf(" Sub20 is not initialized, Please initialize sub20 first !!! " ) ;
      return ;
   }

   /* Read current SPI configuration */
   printf(" ========================== \n") ;
   int spi_cfg = 0 ;

   /* Configure SPI */
   printf(" SPI_CFG = %d \n", SPI_ENABLE|SPI_CPOL_RISE|SPI_SMPL_SETUP|SPI_MSB_FIRST|SPI_CLK_250KHZ ) ;
   int rc = sub_spi_config( hd_, SPI_ENABLE|SPI_CPOL_RISE|SPI_SMPL_SETUP|SPI_MSB_FIRST|SPI_CLK_250KHZ, 0 ) ;
   sub_spi_config( hd_, 0, &spi_cfg );
   if ( rc ==0 ) printf(" SPI configured (%d) \n", spi_cfg ) ;


}

// read the register of EEPROM (RDSR)
void uHTRCtrlMezzInterface::checkEEPROMRegister( sub_handle& hd_ ){

   // Select spi targets - Always SS0 (because using DB9 connector )  
   char wbuff[8] ;
   char rbuff[8] ;

   // Read the EEPROM Re , RDID = 0x9F
   wbuff[0] = 0x05 ; 
   int rc = sub_spi_transfer( hd_, wbuff, 0, 1 , SS_CONF(0,SS_L) ) ;
   if ( rc !=0 ) printf(" SPI Write fail \n" ) ;

   rc = sub_spi_transfer( hd_, 0, rbuff, 1 , SS_CONF(0,SS_LO) ) ;
   if ( rc !=0 ) printf(" SPI READ fail - error code :%d \n", rc ) ;
   if ( rc ==0 ) { 
          printf(" EEPROM Register:        [%X] \n", rbuff[0] ) ;
   }
}

void uHTRCtrlMezzInterface::readEEPROM( sub_handle& hd_ ) {

     vector<char> output ;
     int readSize   = 64 ;

     int k = 0 ;
     vector<char> instruct(3) ;
     instruct[0] = 0x3 ;
     for ( int j = 0; j < 0x3FFF; j+=readSize ) {
         
	 instruct[1] = ( j >>8 )&0xFF ;
	 instruct[2] = ( j >>0 )&0xFF ;
         spi_read( hd_ , instruct, output, readSize ) ;
         k++ ;
     }

     string imageFile = tool_readline(" EEPROM Readout Filename : " ) ;
     FILE* pfile = fopen( imageFile.c_str(), "w" ) ; 

     for ( int i=0; i< output.size(); i++ ) {
         unsigned char byte1 = ( output[i] >> 4 ) & 0xF ;
         unsigned char byte2 = output[i] & 0xF ;

         fprintf(pfile, "%X%X", byte1, byte2 );
         //fprintf(pfile, "%2X", output[i] );
         if ( i%16 == 15 ) fprintf(pfile, "\n") ;
     }
     fclose( pfile ) ;

}

// read the Flash memory ID - a cross-check for spi operation using FLASH mezzanine
void uHTRCtrlMezzInterface::checkFlashID( sub_handle& hd_ ){

   // Select spi targets - Always SS0 (because using DB9 connector )  
   char wbuff[16] ;
   char rbuff[16] ;

   // Read the Flash ID , RDID = 0x9F
   wbuff[0] = 0x9F ; 
   int rc = sub_spi_transfer( hd_, wbuff, 0, 1 , SS_CONF(0,SS_L) ) ;
   if ( rc !=0 ) printf(" SPI Write fail \n" ) ;

   rc = sub_spi_transfer( hd_, 0, rbuff, 3 , SS_CONF(0,SS_LO) ) ;
   if ( rc !=0 ) printf(" SPI READ fail - error code :%d \n", rc ) ;
   if ( rc ==0 ) { 
          printf(" Flash ID :        [%x] [%x] [%x] \n", rbuff[0], rbuff[1], rbuff[2]) ;
   }
}

void uHTRCtrlMezzInterface::writeFirmware( sub_handle& hd_ ){

     checkFlashID( hd_ ) ;

     // read from MCS file
     vector<unsigned char> source ;
     readMCSFile( source ) ;

     // form the input format 
     string imageFile = tool_readline(" mcs write Filename : " ) ;
     FILE* pfile = fopen( imageFile.c_str(), "w" ) ; 
     vector<unsigned char> input ;
     for ( size_t i=0; i< source.size() ; i+= 2 ) {
         unsigned char theByte = source[i] << 4 ;
         theByte += source[i+1] ; 
         input.push_back( theByte ) ; 

         fprintf(pfile, "%02X", input[ input.size()-1 ] ) ;
         if ( i%32  == 30 ) fprintf(pfile, "\n") ;
     }
     printf(" source size: %d , input size: %d \n", (int)source.size() , (int)input.size() ) ;
     fclose( pfile ) ;

     // write into flash
     int nsectors = ( input.size() / 0x40000 ) + 1;
     int sectorSize = (0x40000) ;
     int writeSize = 256 ;
     printf(" start writing to flash (%d sectors) \n\n", nsectors) ;
     int k = 0 ;
     vector<unsigned char> justIn ;
     vector<unsigned char> fillIn ;
     vector<char> addr(3) ;

     for ( int i = 0; i < nsectors; i++) {
         spi_sector_erase( hd_ , i ) ;
         printf("erased and writing to sector[%d] -> starting addr[%x] -> ", i, i*sectorSize ) ;

         bool sectorDone = true ;
         int nTrial = 0 ;

         do {
            sectorDone = true ;
            justIn.clear() ;
            fillIn.clear() ;
            // write to each page 
            k = i*sectorSize ; 
            for ( int j = i*sectorSize; j <(i+1)*sectorSize; j+=writeSize ) {
                //printf(" writing to sector[%d]_addr[%x] ", i, j ) ;
		addr[0] = ( j >>16 )&0xFF ;
		addr[1] = ( j >>8 )&0xFF ;
		addr[2] =   j &0xFF ;
		spi_write( hd_ , addr, input, writeSize, k ) ;

                for ( int h=0; h < 256; h++ ) {
                    unsigned char bt1 = (input[k+h] >> 4) & 0xF ;
                    unsigned char bt2 = input[k+h]  & 0xF ;
                    fillIn.push_back( bt1 ) ;
                    fillIn.push_back( bt2 ) ;
                }
                k += writeSize ;
            }
            printf(" reading back \n" ) ;
            readFirmwareSector( hd_ , justIn, i ) ;

            // verify the write of each sector
            printf(" fill in %d and read out %d \n", (int)fillIn.size() , (int)justIn.size() ) ;
            int nCorr(0), nBad(0) ;
            vector<int> badPages ;

            for ( size_t m=0; m < justIn.size() ; m++ ) {
                if ( justIn[m] != fillIn[m] ) {
                   sectorDone = false ;
                   int bpsz = (int)badPages.size() ;
                   if ( bpsz < 1 )  badPages.push_back( m/512 ) ;
                   if ( bpsz > 0 && badPages[ bpsz-1 ] != m/512 ) badPages.push_back( m/512 ) ;
                   nBad++ ;
                   //printf("  .... flash writing fail at sector [%d] !! \n", i  ) ;
                   //break ;
                }  
                else {
                   nCorr++ ;
                } 
            }

            if ( sectorDone ) printf(" sector done  \n") ;
            //else  printf(" %.3f good and %.3f bad \n", (float)nCorr/(float)(nCorr+ nBad), (float)nBad/(float)(nCorr+ nBad) ) ;
           
            //printf(" bad pages: " ) ;
            //for ( size_t p=0; p < badPages.size() ; p++ )
            //    printf("%d, ", badPages[p] ) ; 

            nTrial++ ;
            if ( nTrial > 1 ) break ;
         } while ( !sectorDone ) ;
         
     }
     printf("  ... write cycle finished \n" ) ;

}

void uHTRCtrlMezzInterface::readFirmwareSector( sub_handle& hd_, vector<unsigned char>& inFlash, int iSector, int iPage ){

     // read firmware from flash
     vector<char> output ;
     //int nsectors   = 26 ;
     int sectorSize = (0x40000) ;
     int readSize   = 256 ;
     //printf(" start reading flash \n") ;
     //for ( int i = 0; i < nsectors; i++) {
         //if ( iSector >= 0 && i != iSector ) continue ;
     int i = iSector ;
     int k = 0 ;
     vector<char> instruct(4) ;
     instruct[0] = 0x3 ;
     for ( int j = i*sectorSize; j <(i+1)*sectorSize; j+=readSize ) {
         if ( iPage >= 0 && k != iPage ) continue ;
         
	 instruct[1] = ( j >>16 )&0xFF ;
	 instruct[2] = ( j >>8 )&0xFF ;
	 instruct[3] =   j &0xFF ;
         spi_read( hd_ , instruct, output, readSize ) ;
         k++ ;
     }
     //}
     //printf(" size of image from flash : %d \n", (int)output.size() ) ;

     inFlash.clear() ;
     for ( size_t j=0 ; j < output.size() ; j++ ) { 
         unsigned char byte1 = ( output[j] >> 4 ) & 0xF ;
         unsigned char byte2 = output[j] & 0xF ;
         inFlash.push_back( byte1 ) ;
         inFlash.push_back( byte2 ) ;
     }
}

bool uHTRCtrlMezzInterface::verifyFirmware( sub_handle& hd_ ){

     checkFlashID( hd_ ) ;

     // read from MCS file
     vector<unsigned char> source ;
     readMCSFile( source ) ;

     string imageFile1 = tool_readline(" mcs readout Filename : " ) ;
     FILE* pfile1 = fopen( imageFile1.c_str(), "w" ) ; 
     for ( size_t i=0; i< source.size() ; i++) {
         fprintf(pfile1, "%X", source[i] ) ;
         if ( i%32  == 31 ) fprintf(pfile1, "\n") ;
     }
     fclose( pfile1 ) ;
     int nSector = source.size()/(0x80000) + 1 ;
     printf(" N of sector = %d \n\n", nSector ) ;

     // read from Flash
     string imageFile = tool_readline(" Flash Readout Filename : " ) ;
     FILE* pfile = fopen( imageFile.c_str(), "w" ) ; 

     vector<unsigned char> inFlash ;
     vector<unsigned char> eachSector ;
     for ( int i=0; i< nSector; i++ ) {
         printf(" reading sector[%d] \n", i ) ;
         readFirmwareSector( hd_ , eachSector, i ) ;
         for ( size_t j=0 ; j < eachSector.size() ; j++ ) { 
             fprintf(pfile, "%X", eachSector[j] );
             inFlash.push_back( eachSector[j] ) ;
             if ( j%32 == 31 ) fprintf(pfile, "\n") ;
          }
     }
     fclose( pfile ) ;

     // verify
     string imageFile2 = tool_readline(" MisMatch output : " ) ;
     FILE* pfile2 = fopen( imageFile2.c_str(), "w" ) ; 
     int ln = 0 ;
     bool allMatch = true ;
     bool corrline = true ;
     //for ( size_t i=0 ; i < inFlash.size(); i++ ) {
     for ( size_t i=0 ; i < source.size(); i++ ) {
         if ( i%16 == 0 ) {
            ln++ ;
            corrline = true ;
         }

         if ( inFlash[i] != source[i] ) {
            corrline = false ;
            //fprintf(pfile2, " Mismatch at Line : [%d] , pos [%u] -> ( %X : %X )\n", ln, i%16, inFlash[i], source[i] ) ;
            allMatch = false ;
         }
         if ( i%16 == 15 && !corrline ) fprintf(pfile2, " Mismatch at Line : [%d] \n", ln) ;      
     }
     fclose( pfile2 ) ;

     return allMatch ;
} 

void uHTRCtrlMezzInterface::readMCSFile( vector<unsigned char>& source ) {

     vector<string> lines ;
     readMCSFile( lines ) ;
     source.clear() ;
     for ( size_t i=1; i< lines.size() ; i++) {
         string theLine = lines[i] ;
         if ( (uint8_t)Char2Hex( theLine[1]) != 1 ) continue ;  // exclude non-data-recording content
         for ( size_t j=9; j < theLine.length() -2 ; j++ ) {
             source.push_back( Char2Hex( theLine[j]) ) ;
         }
     }
}

void uHTRCtrlMezzInterface::readMCSFile( vector<string>& lines ) {

     lines.clear() ;
     // read firmware from mcs file
     string mcsFile = tool_readline(" MCS File : " ) ;
     FILE* pfile = fopen(mcsFile.c_str(), "r" ) ; 

     if (pfile==0) {
        printf( "  \nError opening '%s'\n",mcsFile.c_str() );
     } else { 

       char buffer[1024];
       while ( !feof(pfile) ) {
  	     buffer[0]=0;

             fgets(buffer,1000,pfile);

             if (buffer[0]!=0) {
                for (int i=strlen(buffer)-1; i>=0; i--) {
                    if (isspace(buffer[i])) buffer[i]=0;
                    else break;
                }
             }
             if (strlen(buffer)>4)   lines.push_back(buffer);

       }
     }
     fclose( pfile );

}

void uHTRCtrlMezzInterface::printMCSFile( ) {
 
     vector<string> lines ;
     readMCSFile( lines ) ;
     printf("size of lines : %d \n", (int)lines.size() ) ;

     for ( size_t i=0; i< lines.size() ; i++) {
     
         string theLine = lines[i] ;
         for ( size_t j=0; j < theLine.length() ; j++ ) {
             printf("%c ", theLine[j] ) ;
         }
         printf("\n") ;
         /*
         for ( size_t j=0; j < theLine.length() ; j++ ) {
             unsigned char hexChar = Char2Hex( theLine[j] ) ;
             printf("%2x", (uint8_t) hexChar ) ;
         }
         printf("\n") ;
         */
     }

}


void uHTRCtrlMezzInterface::spi_read( sub_handle& hd_ , vector<char> instruct, vector<char>& output , int readsize  ) {

   if ( hd_ == 0 ) {
      printf(" Sub20 is not initialized, Please initialize sub20 first !!! " ) ;
      return ;
   }

   char wbuff[128] ;
   char rbuff[256] ;

   // Read command (0x3) + 24 bit address
   /*
   wbuff[0] = 0x3 ;
   wbuff[1] = (addr_base >>16)&0xFF ;
   wbuff[2] = (addr_base >>8)&0xFF ;
   wbuff[3] =  addr_base &0xFF ;
   */
   int sz_i = (int) instruct.size() ; 
   for ( int i=0; i < sz_i ; i++ )
       wbuff[i] = instruct[i] ;
   

   int rc = sub_spi_transfer( hd_, wbuff, 0, sz_i , SS_CONF(0,SS_L) ) ;
   if ( rc !=0 ) printf(" SPI Write fail \n" ) ;

   rc = sub_spi_transfer( hd_, 0, rbuff, readsize , SS_CONF(0,SS_LO) ) ;
   if ( rc !=0 ) printf(" SPI READ fail - error code :%d \n", rc ) ;
   if ( rc ==0 ) { 
        
      for ( int i=0 ; i < readsize ; i++ ) {
          output.push_back( (unsigned char)rbuff[i] ) ;
          //printf("%02X ", (unsigned char)rbuff[i] ) ;
          //if ( i%16 ==15  ) printf("\n") ;
      }
      //printf("\n") ;
   }

}

// execute sector_erase (SE) , size of sector is 0x40000 bytes
void uHTRCtrlMezzInterface::spi_sector_erase( sub_handle& hd_, int iSector ) {

   if ( hd_ == 0 ) {
      printf(" Sub20 is not initialized, Please initialize sub20 first !!! " ) ;
      return ;
   }

   // Write Sequence : 
   // (1) Write Enable : WREN 
   // (2) Erase the memory(all bits) to 1s (0xFF) : SE 
   // (3) Page program : PP + 3bytes address + data(>=1 bytes , <= 256 bytes )
   static const uint8_t CMD_WREN = 0x06;   // write enable
   static const uint8_t CMD_SE   = 0xD8;   // sector erase

   char wbuff[260] = { 0 };

   bool running = false ;
   bool isWEL   = false ;
   bool isWP    = false ;

   // (1) write enable
   wbuff[0] =  CMD_WREN ; 
   int rc = sub_spi_transfer( hd_, wbuff, 0, 1 , SS_CONF(0,SS_LO) ) ;
   if ( rc !=0 ) printf(" SPI Write Enable fail \n" ) ;
   isWEL   = spi_status( hd_ , 1 ) ;
   if ( isWEL ) printf(" WEL before writing \n" ) ;
   else         printf(" WEL fail before writing \n" ) ;

   // (2) sector erase
   wbuff[0] =  CMD_SE ; 
   wbuff[1] = 3 + (iSector*4) ;
   wbuff[2] = 0xFF ;
   wbuff[3] = 0xFF ;
   rc = sub_spi_transfer( hd_, wbuff, 0, 4 , SS_CONF(0,SS_LO) ) ;
   if ( rc !=0 ) printf(" SPI Sector Erase fail \n" ) ;

   // Check the status register 
   do {
      running = spi_status( hd_ , 0 ) ;
      isWEL   = spi_status( hd_ , 1 ) ;
   } while( running ) ;
   usleep(100000) ;

}

// write a page (256 bytes )
void uHTRCtrlMezzInterface::spi_write( sub_handle& hd_, vector<char> addr , vector<unsigned char>& input, int writeSize, int input_i ) {

   if ( hd_ == 0 ) {
      printf(" Sub20 is not initialized, Please initialize sub20 first !!! " ) ;
      return ;
   }

   // Write Sequence : 
   // (1) Write Enable : WREN 
   // (2) Erase the memory(all bits) to 1s (0xFF) : SE 
   // (3) Page program : PP + 3bytes address + data(>=1 bytes , <= 256 bytes )
   static const uint8_t CMD_WREN = 0x06;   // write enable
   static const uint8_t CMD_PP   = 0x02;   // page program
   static const uint8_t CMD_WRDI = 0x04;   // write disable

   bool running = false ;
   bool isWEL   = false ;
   bool isWP    = false ;
   char wbuff[512] = { 0 };

   // (3) Page programming 
   //printf(" begin to write \n") ;
   wbuff[0] =  CMD_WREN ; 
   int rc = sub_spi_transfer( hd_, wbuff, 0, 1 , SS_CONF(0,SS_LO) ) ;
   if ( rc !=0 ) printf(" SPI Write Enable fail \n" ) ;
   isWEL   = spi_status( hd_ , 1 ) ;
   
   wbuff[0] =  CMD_PP ;  
   int sz_a = (int)addr.size() ;
   for ( int i=0 ; i < sz_a ; i++ ) 
       wbuff[i+1] = addr[i] ;

   //printf(" input size = %d \n", input.size() ) ;
   for ( int i=0; i < writeSize ; i++ ) {
       wbuff[i+sz_a] = 0 ;
       if ( (i + input_i) < input.size() ) wbuff[i+sz_a] = input[i + input_i] ;
       //printf("  ... wbuff[%d] = %2X \n", i, input[i+input_i] ) ;
   }
   rc = sub_spi_transfer( hd_, wbuff, 0, writeSize+sz_a+1 , SS_CONF(0,SS_LO) ) ;
   if ( rc !=0 ) printf(" SPI Page programming fail - error code :%d \n", rc ) ;

   // Check the status register 
   do {
      running = spi_status( hd_, 0 ) ;
   } while( running ) ;

   // (period of clk = 4 micro-sec) * ( 8bits/byte) * ( size of write data + address + command + 10 ) 
   int tt = 4*8*(writeSize+sz_a+1+10) ; 
   usleep( tt ) ;

}

bool uHTRCtrlMezzInterface::spi_status( sub_handle& hd_, int bitPos ) {

   static const uint8_t CMD_RDSR = 0x05;  // read status register 
   char wbuff[16] = { 0 };
   char rbuff[16] = { 0 };

   bool running = false ; 
   wbuff[0] = CMD_RDSR ;
   int rc = sub_spi_transfer( hd_, wbuff, rbuff, 1 , SS_CONF(0,SS_LO) ) ;
   //rc = sub_spi_transfer( hd_, 0, rbuff, 1 , SS_CONF(0,SS_LO) ) ;
   if ( rbuff[0] !=0 ) printf(" SPI Status(%d)  = %2X \n", bitPos , (unsigned char)rbuff[0] ) ;
   if ( ( rbuff[0] >> bitPos ) & 0x1 ) { 
      if ( bitPos == 0 )  printf(" ... Busy ... \n") ;
      if ( bitPos == 1 )  printf(" ... Write Enable Latch ...\n") ;
      if ( bitPos == 7 )  printf(" ... Write Protection ...\n") ;
      running = true ;
   }
   //   printf("\n") ;

   return running ;
}

// Tranform Char(0 ~ F/f) to Hex value , other charactor will be set to 0 
unsigned char uHTRCtrlMezzInterface::Char2Hex(unsigned char c) {

      char val5 = (c-'a'+0xa) ;

      char val3 = (c-'A'+0xa) ;
      char val4 =  (c>='a' && c<='f') ? val5 : 0 ;
    
      char val1 = (c-'0') ;
      char val2 = (c>='A' && c<='F') ? val3 : val4 ;

      return ( (c>='0' && c<='9')? val1 : val2 );
}

