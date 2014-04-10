#include <cstdlib>
#include <cstdio>
#include <unistd.h> 
#include <getopt.h>
#include <time.h>
#include <map>
#include <string>
#include "sub20tool.h"
#include <readline/readline.h>
#include <readline/history.h>



void help() ;
string tool_readline() ;

void help()
{
    printf("Usage: ./sub20tool.exe modeFlags [options]\n");
    printf(" -f [script_file],   run script file\n");
    printf(" -c                  Scan i2c slaves\n");
    printf(" -h                  Showing this help menu\n");
}

string tool_readline() {

    string retval;

    char* input = readline( "> " );
    if ( input && *input ) add_history( input ) ;

    retval   = string( input );
    free( input );

    //if (retval.empty()) retval=defval;
    return retval;
}

int main(int argc, char* argv[]) {
    /*
    if (argc < 2) {
        help();
        return 0;
    }
    */

    // Initialize things
    sub20tool* sub = new sub20tool() ;
    char*  cmdfile ;
    bool runScript = false ;

    sub->InitializeSub20();
    sub->set_sub20_frequence();

    // Get options
    for (int i=1; i< argc ; i++) {
        string opt = argv[i] ;
        if ( opt == "-f" ) {
            runScript = true ;
            cmdfile = argv[i+1] ;
            printf(" runCMD file : %s \n", cmdfile ) ;
        }
        if ( opt == "-c" ) {
           sub->scan_i2c_slaves();      
        }
        if ( opt == "-h" )  help() ;
    }


    if ( runScript ) {
       sub->RunCmdFile(cmdfile);
    } else {

       // begin read/write operations 
       printf( "<<< Interactive mode ( h for help ) >>> \n");
       bool keepRunning = true ;
       vector<string> cmds ;
       do {
	  string cmd_reg = tool_readline() ;
	  if ( strncasecmp( cmd_reg.c_str() , "exit", 4 ) == 0 ) break ;
	  if ( strncasecmp( cmd_reg.c_str() , "quit", 4 ) == 0 ) break ;
	  if ( strncasecmp( cmd_reg.c_str() , "q",    1 ) == 0 ) break ;

	  // Get commands 
	  cmds.clear() ;
	  char *pch ;
	  char cpline[500];
	  strcpy(cpline, cmd_reg.c_str() );
	  pch = strtok( cpline , " ," )  ;
	  while ( pch != NULL ) {
              //printf ("[%s] \n",pch);
                cmds.push_back( (string)pch ) ;
                pch = strtok( NULL , " ," ) ;
          }

          // Action 
          sub->action(cmds);

       } while ( keepRunning ) ;
    }

    return 0 ;

}


bool sub20tool::InitializeSub20() {

  /*    bool found_s20 = false ;
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
 
    return found_s20 ;*/
  return true;

}

// set operation frequence of i2c communication for sub20 , Maximum is 444444 Hz
void sub20tool::set_sub20_frequence(int freq) {

  /*if ( hd_ == 0 ) {
      printf(" Sub20 is not initialized, Please initialize sub20 first !!! " ) ;
      return ;
   }

   int f_ =   sub_i2c_freq( hd_ , &freq ) ;
   if ( f_ == 0 ) printf(" frequence is set to %d Hz! (Maximum : 4444444 )\n", freq ) ;
   else           printf(" frequence set fail ! \n") ; */

}

void sub20tool::scan_i2c_slaves() {

  /*if ( hd_ == 0 ) {
      printf(" Sub20 is not initialized, Please initialize sub20 first !!! " ) ;
      return ;
   }

   // target slave - i2c MUX
   int nSlave = 0 ;
   char sAddr[128];
   int s_ = sub_i2c_scan( hd_, &nSlave, sAddr ) ;
   printf("  ==> i2c(%d) scan : %d  slaves found ! \n", s_ , nSlave  ) ;

   for ( int j =0 ; j < nSlave ; j++ )
   printf("    --> addr[%d]: %x \n", j+1, sAddr[j] ) ;*/
}

void sub20tool::read(int addr, int nbyte ) {

   char rbuff[256] ;
   int r_ = i2c->i2c_read(addr, rbuff, nbyte );
   printf("  Read from 0x[%x] : ", addr ) ;
   for ( int i= 0 ; i < nbyte ; i++ ) {
       printf("[%x] ", rbuff[i] ) ;
       if ( i%4 ==3 ) printf(" \n                   ") ;
   }
   printf("\n") ;

   if ( r_ != 0 ) printf(" i2c read error \n") ;

}

void sub20tool::write(vector<string>& cmdv ) {

   printf(" cmdv size: %d \n", (int)cmdv.size() ) ;
   int addr = (int)strtol(cmdv[1].c_str(), 0,0) ;

   char wbuff[256] ;
   for ( size_t i=2; i< cmdv.size() && i < 256 ; i++ ) 
       wbuff[i] = (int)strtol( cmdv[i].c_str(),0,0)  ;

   int wsz =  ( cmdv.size() < 256 ) ? cmdv.size() -2 : 256 ; 

   int w_ = i2c->i2c_write(addr, wbuff, wsz );

   printf("  Write to 0x[%x] : ", addr ) ;
   for ( size_t i=2; i< cmdv.size() && i < 256 ; i++ )  {
       printf( "%X ", wbuff[i] ) ;
       if ( i%4 ==3 ) printf(" \n                   ") ;
   }
   printf("\n") ;

   if ( w_ != 0 ) printf(" i2c write error \n") ;

}

void sub20tool::action(vector<string>& cmdv ) {


    if ( cmdv.size() < 1 ) printf( "No command \n" ) ;

    if ( cmdv[0] == "help" || cmdv[0] == "h" )  sub20_help() ;

    if ( cmdv[0] == "c" ) scan_i2c_slaves();
    if ( cmdv[0] == "s" ) {
       if   ( cmdv.size() == 2 ) usleep( (int)strtol( cmdv[1].c_str(), 0, 0)  ) ;
       else printf(" sleep usage: s [time in micro-second] \n" ) ;
    }

    if ( cmdv[0] == "r" ) {
       if ( cmdv.size() == 2 )       read((int)strtol(cmdv[1].c_str(), 0,0)  ) ;
       else if ( cmdv.size() == 3 )  read((int)strtol(cmdv[1].c_str(), 0,0) , (int)strtol( cmdv[2].c_str(),0,0) ) ;
       else                          printf(" read error - usage: r [addr] [N_read]\n" ) ;
    }
 
    
    if ( cmdv[0] == "w" ) {
       printf(" write : %s %s %s \n", cmdv[0].c_str(), cmdv[1].c_str(), cmdv[2].c_str() ) ;
       if ( cmdv.size() >= 3 )  write(cmdv ) ;
       else printf(" writing error - usage: w [addr] [value]\n" ) ;
    }
    
    cmdv.clear() ;
}

void sub20tool::RunCmdFile(std::string theFile ){

    FILE* cmdfile = fopen( theFile.c_str() ,"r");
    if ( cmdfile == NULL )  printf("Unable to open '%s' ", theFile.c_str()  );

    // Get each line in the command file 
    vector<string> cmdlist ;
    vector<string> cmdv ;
    char buffer[256] ;  

    while ( !feof(cmdfile) ) { 

        buffer[0] = 0 ;  // clean up the buffer 
        fgets( buffer, 256, cmdfile ) ;
        if ( feof(cmdfile) ) break ;

        // Split line into several tokens and store in the cmd 
        char* pch ; 
        pch = strtok( buffer, " ," ) ;
        //if ( pch != NULL ) cmdv.push_back( pch ) ;
        while ( pch != NULL ) {
              //cout<<" > "<< pch << endl ; 
              if ( pch != NULL ) cmdv.push_back( pch ) ;
              pch = strtok( NULL, " ," ) ;
        }
        action(cmdv);
    }

}

void sub20tool::sub20_help() {
    printf(" read  : > r [address] [N_Read]\n");
    printf(" write : > w [address] [val1], [val2], [val3] ... \n");
    printf(" sleep : > s [time in micro-second]        \n");
    printf(" scan  : > c         \n");
    printf(" exit  : > q         \n");
    printf(" help  : > h         \n");
}
