#include <cstdlib>
#include <cstdio>
#include <unistd.h> 
#include <getopt.h>
#include <time.h>
#include <map>
#include <string>
#include <vector>
#include <signal.h>
#include <curses.h>
#include "io.h"

#include "uHTRPowerMezzInterface.h"
#include "uHTRPowerMezzMenu.h"
#include "uHTRMezzanines.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>


const int TimeToN = 6;

int N_margup = 0; //30 min
int N_margdn = 0; //30 min
int N_nom    = 57600; //160 hours 
int N_nom_lh = 0; //19 hours 
const bool will_sleep = true;

double PM_VOUT_NOM = 1.0;
const double PM_VA_NOM = 12.0;
const double PM_VB_NOM = 12.0;

double APM_VOUT_NOM = 1.8;
const double APM_VC_NOM = 12.0;

Mezzanines * active_mezzanines = NULL;
uHTRPowerMezzInterface * active_s20 = NULL;

int test_mezzanines(uHTRPowerMezzInterface& s20, Mezzanines * mezzanines);
void help();

unsigned int in_[] = {0x6e6f420c, 0x72756f6a, 0x20202021, 0x00202020, 0x6d614d0c, 0x64206e61, 0x6e655265, 0x00202065,
    0x7473650c, 0x20697320, 0x6f726720, 0x00202c73, 0x6175710c, 0x2020646e, 0x6c6c6520, 0x00202065,
    0x756f620c, 0x20206567, 0x20656c20, 0x006c7563, 0x6c6c650c, 0x720a2065, 0x69757165, 0x00747265,
    0x7565640c, 0x760a2078, 0x6761796f, 0x00217365, 0x2020200c, 0x20202020};

void help()
{
    printf("Usage: ./uHTR_PowerMod_Test.exe modeFlags [options]\n");
    printf("Mode Flags - These can be used in conjunction but will always evaluate in the order listed below.\n");
    printf(" --echo,         -e   Echo string to sub20\n");
    printf(" --enable,            Enable all Mezzanines\n");
    printf(" --disable,      -d   Disable all Mezzanines\n");
    printf(" --readEeproms,  -r   Read power mezzanine eeproms\n");
    printf(" --labelPM,      -p   Program power mezzanine eeprom\n");
    printf(" --labelAPM,     -a   Program aux power mezzanine eeprom\n");
    printf(" --runTest,      -t   Run power mezzanine test\n");
    printf("Option Flags\n");
    printf(" --margUpTime,   -M   Sub20 number (default is 0)\n");
    printf(" --margUpTime,   -U   Margin up test length (in min)\n");
    printf(" --margDnTime,   -D   Margin down test length (in min)\n");
    printf(" --nomTime,      -T   Nominal test length (in min)\n");
    printf(" --highLoadTime, -H   High load test length (in min)\n");
    printf(" --pmVolt,       -P   PM voltage (in V)\n");
    printf(" --apmVolt,      -A   APM voltage (in V)\n");
    printf(" --board-id,     -I   board ID to match in config file\n");
    printf(" --testSite,     -S   Test site name (<= 15 characters)\n");
    printf(" --testerName,   -N   Tester name (<= 15 characters\n");
    printf(" --version1,     -V   Tester name (<= 15 characters\n");
    printf(" --configFile,   -F   Configuration file (default \"configuration.txt\")\n");
    printf(" --skipSNBlock,       The SN assignment will skip a block of SNs of specified size\n");
    printf(" --help,         -h   help (This menu)\n");
}

extern std::string parse_to(std::string &buff, std::string del = ",")
{
    size_t pos = buff.find(del);
    if(pos == std::string::npos) pos = buff.size();
    std::string ret = buff.substr(0, pos);
    pos = buff.find_first_not_of(del + " \t", pos);
    buff.erase(0, pos);
    return ret;
}

//if interupt turn off loads and stop test
void  INThandler(int sig)
{
    signal(sig, SIG_IGN);

    io::printf("\n***************************************** \n");
    io::printf(  "**** Turning off power to Mezzanines **** \n");
    io::printf(  "***************************************** \n");
    active_mezzanines->setPrimaryLoad(false, false);
    active_mezzanines->setSecondaryLoad(false, false, false, false);
    active_mezzanines->setRun(false);

    //active_s20->stopTest();
    exit(0);
}

int main(int argc, char* argv[])
{
    int opt;
    int option_index = 0;
    static struct option long_options[] = {
        {"readEeproms",       no_argument, 0, 'r'},
        {"runTest",           no_argument, 0, 't'},
        {"margUpTime",  required_argument, 0, 'U'},
        {"margDnTime",  required_argument, 0, 'D'},
        {"nomTime",     required_argument, 0, 'T'},
        {"highLoadTime",required_argument, 0, 'H'},
        {"pmVolt",      required_argument, 0, 'P'},
        {"apmVolt",     required_argument, 0, 'A'},
        {"labelPM",           no_argument, 0, 'a'},
        {"labelAPM",          no_argument, 0, 'p'},
        {"testSite",    required_argument, 0, 'S'},
        {"testerName",  required_argument, 0, 'N'},
        {"skipSNBlock", required_argument, 0, 'B'},
        {"sub20Num",    required_argument, 0, 'M'},
        {"echo",        required_argument, 0, 'e'},
        {"version1",          no_argument, 0, 'V'},
        {"board-id",    required_argument, 0, 'I'},
        {"enable",            no_argument, 0, 's'},
        {"disable",           no_argument, 0, 'd'},
        {"configFile",  required_argument, 0, 'F'},
        {"r00t",              no_argument, 0, 'k'},
        {"help",              no_argument, 0, 'h'}
    };

    bool runTest = false, labelPM = false, labelAPM = false, readEeproms = false, siteSet = false, testerSet = false;
    bool echo = false, k = false, isV2 = true, pSet = false, aSet = false, enable = false, disable = false;
    bool interactive = false;
    char site[16], tester[16] ="", echoString[32], cFileName[128] = "configuration.txt";
    unsigned int sbSize = 0, sub20Num = 0;
    int boardID = 0;

    if(argc == 1)
    {
        interactive = true;
    }

    while((opt = getopt_long(argc, argv, "rapthe:dq:F:M:U:D:T:H:P:A:S:N:I:V", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 't':
                runTest = true;
                break;
            case 'U':
                N_margup = int(atoi(optarg) * TimeToN);
                break;
            case 'D':
                N_margdn = int(atoi(optarg) * TimeToN);
                break;
            case 'T':
                N_nom = int(atoi(optarg) * TimeToN);
                break;
            case 'H':
                N_nom_lh = int(atoi(optarg) * TimeToN);
                break;
            case 'P':
                PM_VOUT_NOM = atof(optarg);
                pSet = true;
                break;
            case 'A':
                APM_VOUT_NOM = atof(optarg);
                aSet = true;
                break;
            case 'S':
                if(strlen(optarg) > 15)
                {
                    printf("Site name restricted to 15 character.");
                    return 0;
                }
                siteSet = true;
                sprintf(site, "%s", optarg);
                break;
            case 'N':
                if(strlen(optarg) > 15)
                {
                    printf("Tester name restricted to 15 character.");
                    return 0;
                }
                testerSet = true;
                sprintf(tester, "%s", optarg);
                break;
            case 'B':
                sbSize = int(atoi(optarg));
                break;
            case 'M':
                sub20Num = int(atoi(optarg));
                break;
            case 'V':
                isV2 = false;
                break;
            case 'p':
                labelPM = true;
                break;
            case 'a':
                labelAPM = true;
                break;
            case 'r':
                readEeproms = true;
                break;
            case 'e':
                echo = true;
                sprintf(echoString,"\f%s",optarg);
                break;
            case 's':
                enable = true;
                break;
            case 'd':
                disable = true;
                break;
            case 'F':
                sprintf(cFileName, "%s", optarg);
                break;
            case 'k':
                k = true;
                break;
            case 'I':
                boardID = int(atoi(optarg));
                break;
            case 'h':
            default:
                help();
                return 0;

            case 'q':
                uHTRPowerMezzInterface s20(sub20Num, isV2, true, "cms1pi","1338");

                int chan = int(atoi(optarg));
                int adc = 0;

                const int RPimuxChan = chan / 10;
                chan %= 10;

                if(chan == 0)      
                {
                    chan = V2_MUX_PM_1_A;
                    adc = 1;
                }
                else if(chan == 1) 
                {
                    chan = V2_MUX_APM_1_6;
                    adc = 2;
                }
                else if(chan == 2) 
                {
                    chan = V2_MUX_PM_1_B;
                    adc = 1;
                }
                else if(chan == 3) 
                {
                    chan = V2_MUX_APM_2_5;
                    adc = 2;
                }
                else if(chan == 4) 
                {
                    chan = V2_MUX_PM_3_3;
                    adc = 1;
                }

                printf("12V:          %8.2f mV\n", 9.2*s20.readSUB20ADC(7 - 2*(RPimuxChan - 1))*1000);
                printf(" 5V:          %8.2f mV\n", 3.7*s20.readSUB20ADC(6 - 2*(RPimuxChan - 1))*1000);
                s20.setMUXChannel(chan, RPimuxChan);
                printf("MUX STATUS: %x\n", s20.getError());
		s20.configGPIO();
                s20.togglePowerMezzs(true);
                printf("TOG STATUS: %x\n", s20.getError());
                if(adc == 2) s20.configADC128();
                sleep(1);
                //double v2 = s20.readMezzADC(adc, 1);
                if(adc == 2) printf("CFG STATUS: %x\n", s20.getError()); 
                sleep(1);
                if(adc == 1)
                {
                    double v2 = s20.readMezzADC(1, 1);
                    printf("I_A STATUS: %x %8.2f mA\n", s20.getError(), 2.5 * v2);
                    v2 = s20.readMezzADC(1, 2);
                    printf("I_B STATUS: %x %8.2f mA\n", s20.getError(), 2.5 * v2);
                }
                else
                {
                    double v2 = s20.readMezzADC(2, 4);
                    printf("I_C STATUS: %x %8.2f mA\n", s20.getError(), 1.5625 * v2);
                }
                double v1 = s20.readMezzADC(0, 2);
                printf("V_O STATUS: %x %8.2f mV\n", s20.getError(), 4.32*v1);
                if(adc == 2) 
                {
                    double V_ADJ = s20.readMezzADC(2, 0);
                    printf("V_ADJA    : %x %8.2f mV\n", s20.getError(), V_ADJ/1.6);
                    V_ADJ = s20.readMezzADC(2, 1);
                    printf("V_ADJB    : %x %8.2f mV\n", s20.getError(), V_ADJ/1.6);
                    V_ADJ = s20.readMezzADC(2, 2);
                    printf("V_ADJC    : %x %8.2f mV\n", s20.getError(), V_ADJ/1.6);
                    V_ADJ = s20.readMezzADC(2, 3);
                    printf("V_ADJD    : %x %8.2f mv\n", s20.getError(), V_ADJ/1.6);
                }
                s20.togglePowerMezzs(false);
                return 0;
        }
    }

    if(!(readEeproms || labelPM || labelAPM || runTest || echo || disable || enable)) interactive = true;

    // Initialize list of mezzanines 
    boost::mutex s20mtx;
    Mezzanines * mezzanines = new Mezzanines(&s20mtx);

    active_mezzanines = mezzanines;

    // Load configuration file    
    char buff[4096];
    std::map<int,std::string> config_lines;
    FILE *cfile = fopen(cFileName, "r");
    if(!cfile)
    {
        printf("Configuration file %s not found!!!\n", cFileName);
        return 0;
    }

    // Grab which adapter is being used for i2c
    char adapter[3] = "";
    char hostname[32] = "localhost";
    char port[8] = "1338";
    int adChan;

    while(!feof(cfile) && (fgets(buff, 4096, cfile)) != NULL)
    {
        if(buff[0] == '#') continue;
        std::string tmp = buff;
        tmp.erase(tmp.size()-1);
        if(tmp.size() < 10) continue;
        unsigned int id = atoi(parse_to(tmp).c_str());
        config_lines[id] = tmp;
    }

    //Do interactive


    if(interactive)
    {
	char responce = '\0';
        uHTRPowerMezzMenu menu(config_lines,isV2,tester);
        for(int loops = 0;true;loops++)
        {
            if( !(loops % 100) ) 
            {
                menu.query_servers();
                menu.display();
                loops = 0;
            }

            boardID = menu.start_test(&responce);
            if(boardID == 0) 
            {
                usleep(100000);
                continue;
            }
            else if(boardID == -1) menu.quit();

	    if(responce == 's')
	    {
		N_margup = 180; //30 min
		N_margdn = 180; //30 min
		N_nom    = 180; //30 min
		N_nom_lh = 180; //30 min
	    }
	    else if(responce == 't')
	    {
		N_margup = 0; //0 min
		N_margdn = 0; //0 min
		N_nom    = 57600; //160 hours 
		N_nom_lh = 0; //0 min
	    }
            runTest = true;

            io::disable_curses();

            time_t rawtime;
            struct tm * timeinfo;
            char buffer [80];

            time (&rawtime);
            timeinfo = localtime (&rawtime);

            strftime (buffer,80,"%F-%R",timeinfo);

            char logname[64];
            sprintf(logname, "log.%02d.%s.txt", boardID,buffer);
            FILE * log = fopen(logname, "w");
            io::Instance()->out = log;
            break;
        }
    }

    std::string config_line;
    std::map<int, std::string>::iterator it = config_lines.find(boardID);

    if(it != config_lines.end()) 
    {
        config_line = it->second;
    }
    else 
    {
        printf("BoardID not found. Specify with -I# (or add board to config file)\n");
        return 0;
    }

    adChan = atoi(parse_to(config_line).c_str());
    sprintf(adapter, "%s", parse_to(config_line).c_str());
    sprintf(hostname,    "%s", parse_to(config_line,":").c_str());
    sprintf(port,    "%s", parse_to(config_line).c_str());
    io::printf("boardID:  %d\n" , boardID);
    io::printf("adChan:   %d\n" , adChan);
    io::printf("adapter:  %s\n" , adapter);
    io::printf("hostname: %s\n" , hostname);
    io::printf("port:     %s\n" , port);

    // Initialize i2c device
    bool isRPi = false;
    if(strcmp(adapter, "RPi") == 0) isRPi = true;
    else if(strcmp(adapter, "S20") == 0) isRPi = false;
    else 
    {
        io::printf("i2c adapter: %s is invalid!!! (Options are \"RPi\" and \"S20\")\n", adapter);
        return 0;
    }
    uHTRPowerMezzInterface s20(sub20Num, isV2, isRPi,hostname,port);
    s20.init();


    // Grab mezzanines 
    char slot[32];
    float voltage;
    bool mezzIn[]= {false,false,false,false,false};
    for(std::string mezz = parse_to(config_line); mezz != ""; mezz = parse_to(config_line))
    {
        if(sscanf(mezz.c_str(), "%s %f\n", slot, &voltage) == 2)
        {
            if(isRPi && (adChan < 1 || adChan > 6)) 
            {
                io::printf("Invalid RPi adapter channel (1 - 6 are valid)!!!\n");
                return 0;
            }
            else if(!isRPi)
            {
                adChan = -1; // unnecessary for s20 adapter
            }

            if(strcmp(slot, "PM_3_3") == 0 && !mezzIn[0])
            {
                mezzanines->push_back(new PM(s20, V2_MUX_PM_3_3, voltage, true, adChan));
                mezzIn[0]=true;
            }
            else if(strcmp(slot, "APM_2_5") == 0 && !mezzIn[1])
            {
                mezzanines->push_back(new APM(s20, V2_MUX_APM_2_5, voltage, true, adChan));
                mezzIn[1]=true;
            }
            else if(strcmp(slot, "PM_1_B") == 0 && !mezzIn[2])
            {
                mezzanines->push_back(new PM(s20, V2_MUX_PM_1_B, voltage, true, adChan));
                mezzIn[2]=true;
            }
            else if(strcmp(slot, "APM_1_6") == 0 && !mezzIn[3])
            {
                mezzanines->push_back(new APM(s20, V2_MUX_APM_1_6, voltage, true, adChan));
                mezzIn[3]=true;
            }
            else if(strcmp(slot, "PM_1_A") == 0 && !mezzIn[4])
            {
                mezzanines->push_back(new PM(s20, V2_MUX_PM_1_A, voltage, true, adChan));
                mezzIn[4]=true;
            }
            else
            {
                io::printf("Invalid or duplicate Mezzanine slot: %s (options are PM_3_3, APM_2_5, PM_1_B, APM_1_6, PM_1_A)!!!\n", slot);
            }
        }
    }
    fclose(cfile);

    mezzanines->init();

    if(echo)
    {
        s20.updateSUB20Display(echoString);

        if(readEeproms || labelPM || labelAPM || runTest) sleep(10);
    }

    if(k)
    {
        for(unsigned int i = 0; i < sizeof(in_) / sizeof(unsigned int); i += 4)
        {
            s20.updateSUB20Display((char *)(in_ + i));
            sleep(1);
        }
    }

    if(enable)
    {
        mezzanines->setPrimaryLoad(false, false);
        mezzanines->setSecondaryLoad(false, false, false, false);
        mezzanines->setRun(true);
    }

    if(disable)
    {
        mezzanines->disableMezzanines();
    }

    if(readEeproms)
    {
        mezzanines->readEeprom();
        s20.updateSUB20Display("\fEEPROMs\nread");
    }

    // read in SN file
    if(labelPM || labelAPM)
    {
        if(!siteSet && !testerSet)
        {
            io::printf("Site name and tester name must both be specified to program eeprom (-h for options)\n");
            return 0;
        }
    }

    if(labelPM)  mezzanines->labelPM(tester, site);

    if(labelAPM) mezzanines->labelAPM(tester, site);

    if(runTest)
    {
        //Handle Signal
        struct sigaction sa;

        sa.sa_handler = INThandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART; /* Restart functions if
                                     interrupted by handler */
        if (sigaction(SIGINT, &sa, NULL) == -1)
        {
            io::printf("handle this error?\n");
        }

        s20.startTest(getpid());
        int retval = test_mezzanines(s20, mezzanines);
        char retmessage[32];
        Mezzanine::Summary::translateStatus(retval, retmessage);
        if(retval)
        {
            io::printf("\nExit with value: %s\n", retmessage);
        }

        ////Make sure everything is off again
        //mezzanines->setRun(false);
        //mezzanines->setPrimaryLoad(false, false);
        //mezzanines->setSecondaryLoad(false, false, false, false);

        s20.stopTest();
    }
}

//======================================================================
// Test procedures for uHTR POWER MODULE and uHTR AUX_POWER MODULE
//======================================================================

int test_mezzanines(uHTRPowerMezzInterface& s20, Mezzanines * mezzanines)
{
    active_s20 = &s20;
    int i, status;
    io::printf("Starting Mezzanine Tests...\n");
    if (mezzanines->empty()) return RETVAL_NO_MEZZ_SPEC;

    int adChan = mezzanines->front()->get_adChan();


    // Turn power mezzanines on
    mezzanines->setRun(true);
    usleep(100000); // wait to ensure mezzaning power up complete

    //====================================================================
    // CHECK SUPPLY VOLTAGES.
    //====================================================================

    if(s20.isV2_)
    {
        // Read voltage from sub20 board

        if(s20.isRPi_)
        {
            // 12 V supply is read through a divide by 9.2
            //
            if(9.2 * s20.readSUB20ADC(7 - 2*(adChan - 1)) < 10.0) return RETVAL_BAD_SUPPLY_VOLTAGE;

            // 5 V supply is read through a dividr by 3.7
            if(3.7 * s20.readSUB20ADC(6 - 2*(adChan - 1)) < 4.5) return RETVAL_BAD_3_3VOLT;

        }
        else
        {
            // 12 V supply is read through a divide by 9.2
            if(9.2 * s20.readSUB20ADC(6) < 10.0) return RETVAL_BAD_SUPPLY_VOLTAGE;

            // 5 V supply is read through a dividr by 3.7
            if(3.7 * s20.readSUB20ADC(7) < 4.5) return RETVAL_BAD_3_3VOLT;
        }
    }
    else
    {
        //--------------------------------------------------------------------
        // Set MUX to channel 1, AUX_POWER MODULE
        //--------------------------------------------------------------------
        // PCA9544A; SLAVE ADDRESS: 1110 000
        s20.setMUXChannel(MUX_AUXPOWERMOD, -1);

        //--------------------------------------------------------------------
        // Read 12V supply voltage. If 12V supply reads less than 8V then 
        // exit with a failure.
        //--------------------------------------------------------------------
        // MCP3428; SLAVE ADDRESS: 1101 010
        // Channel 3: V12 = 18.647 * V_CH3
        // 2.048V reference -> 1 mV per LSB
        if(18.647 * s20.readMezzADC(ADC_28, 3) < 10000.0) return RETVAL_BAD_SUPPLY_VOLTAGE;

        //--------------------------------------------------------------------
        // Read 3.3V supply voltage. If 3.3V supply reads less than 3V then 
        // exit with a failure.
        //--------------------------------------------------------------------
        // MCP3428; SLAVE ADDRESS: 1101 010
        // Channel 4: V3.3 = 18.647 * V_CH4
        // 2.048V reference -> 1 mV per LSB
        if(18.647 * s20.readMezzADC(ADC_28, 4) < 3000.0) return RETVAL_BAD_3_3VOLT;
    }

    // Activate nominal load resistors 
    mezzanines->setPrimaryLoad(true, false);
    mezzanines->setSecondaryLoad(true, true, true, true);
    usleep(100000); //wait for output to stabalize 

    // This thread updates the sub20 display
    boost::thread display(boost::bind(&Mezzanines::displayAndSleep, mezzanines, s20));

    //====================================================================
    // Set LOW voltage margins.
    //====================================================================

    mezzanines->setMargins(-1);

    // Monitor for N loops at 10 seconds per loop.

    io::printf("Margin Down Test:");
    for (i = 0; i < N_margdn; i++)
    {
        boost::thread timer(boost::bind(sleep, 10));
        status = 0;
        //io::printf(".");
        fflush(stdout);
        status |= mezzanines->monitor();
        char statusmessage[32];
        Mezzanine::Summary::translateStatus(status, statusmessage);
        io::printf("%s\n", statusmessage);

        if(!s20.isV2_ && (status & RETVAL_ABORT)) return RETVAL_ABORT;

        // SLEEP 10 seconds.
        if(will_sleep) timer.join();
        if(!mezzanines->arePresent()) return RETVAL_NO_MODLE_DETECTED;
    }
    io::printf("\n");
    mezzanines->print();

    //====================================================================
    // Set HIGH voltage margins.
    //====================================================================

    mezzanines->setMargins(1);

    // Monitor for N loops at 10 seconds per loop.

    io::printf("Margin Up Test:");
    for (i = 0; i < N_margup; i++)
    {
        boost::thread timer(boost::bind(sleep, 10));
        status = 0;
        //io::printf(".");
        fflush(stdout);
        status |= mezzanines->monitor();

        char statusmessage[32];
        Mezzanine::Summary::translateStatus(status, statusmessage);
        io::printf("%s\n", statusmessage);

        if(!s20.isV2_ && (status & RETVAL_ABORT)) return RETVAL_ABORT;

        // SLEEP 10 seconds.
        if(will_sleep) timer.join();
        if(!mezzanines->arePresent()) return RETVAL_NO_MODLE_DETECTED;
    }
    io::printf("\n");
    mezzanines->print();

    //====================================================================
    // Set NOMINAL voltage margins.
    //====================================================================

    mezzanines->setMargins(0);

    // Monitor for N loops at 10 seconds per loop.
    io::printf("Nominal Test:");
    for (i = 0; i < N_nom; i++)
    {
        boost::thread timer(boost::bind(sleep, 10));
        status = 0;
        //io::printf(".");
        fflush(stdout);
        status |= mezzanines->monitor();
        char statusmessage[32];
        Mezzanine::Summary::translateStatus(status, statusmessage);
        io::printf("%s\n", statusmessage);

        if(!s20.isV2_ && (status & RETVAL_ABORT)) return RETVAL_ABORT;

        // SLEEP 10 seconds.
        if(will_sleep) timer.join();

        if(!mezzanines->arePresent()) return RETVAL_NO_MODLE_DETECTED;
    }
    io::printf("\n");
    mezzanines->print();

    //====================================================================
    // Set HIGH LOAD voltage test.
    //====================================================================

    mezzanines->setMargins(0, 2);

    // Set full load
    mezzanines->setPrimaryLoad(true, true);
    mezzanines->setSecondaryLoad(true, true, true, true);
    usleep(100000); //give time for voltage to settle

    // Monitor for N loops at 10 seconds per loop.
    io::printf("High Load Test:");
    for (i = 0; i < N_nom_lh; i++)
    {
        boost::thread timer(boost::bind(sleep, 10));
        status = 0;
        //io::printf(".");
        fflush(stdout);
        status |= mezzanines->monitor();
        char statusmessage[32];
        Mezzanine::Summary::translateStatus(status, statusmessage);
        io::printf("%s\n", statusmessage);

        if(!s20.isV2_ && (status & RETVAL_ABORT)) return RETVAL_ABORT;

        // SLEEP 10 seconds.
        if(will_sleep) timer.join();

        if(!mezzanines->arePresent()) return RETVAL_NO_MODLE_DETECTED;
    }
    io::printf("\n");
    mezzanines->print();

    // End of test cleanup

    // Turn power mezzanines off
    //mezzanines->setRun(false);
    mezzanines->disableMezzanines();

    // Deactivate nominal load resistors 
    //mezzanines->setPrimaryLoad(false, false);
    //mezzanines->setSecondaryLoad(false, false, false, false);

    display.~thread();

    // Set final status to screen
    s20.updateSUB20Display("\fTEST\nCOMPLETE");

    //====================================================================
    // RETURN
    //====================================================================
    return(RETVAL_SUCCESS);
}

