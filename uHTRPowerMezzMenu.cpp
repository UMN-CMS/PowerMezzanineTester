#include "uHTRPowerMezzMenu.h"
#include <curses.h>

extern std::string parseto(std::string &buff, std::string del = ",")
{
    size_t pos = buff.find(del);
    if(pos == std::string::npos) pos = buff.size();
    std::string ret = buff.substr(0, pos);
    pos = buff.find_first_not_of(del + " \t", pos);
    buff.erase(0, pos);
    return ret;
}

uHTRPowerMezzMenu::uHTRPowerMezzMenu(std::map< int, std::string> config_lines, bool isV2)
{
    for (std::map< int, std::string>::iterator it = config_lines.begin();
            it!= config_lines.end();
            it++)
    {
        Board b;
        b.id = it->first;
        std::string config_line = it->second;
        //std::cout << config_line << std::endl;

        b.adChan = atoi(parseto(config_line).c_str());
        sprintf(b.adapter,  "%s", parseto(config_line).c_str());
        sprintf(b.hostname, "%s", parseto(config_line,":").c_str());
        sprintf(b.port,     "%s", parseto(config_line).c_str());

        //std::cout << "boardID: " << b.id << std::endl;
        //std::cout << "adChan: " << b.adChan << std::endl;
        //std::cout << "adapter: " << b.adapter << std::endl;
        //std::cout << "hostname: " << b.hostname << std::endl;
        //std::cout << "port: " << b.port << std::endl;

        // Initialize i2c device
        bool isRPi = false;
        if(strcmp(b.adapter, "RPi") == 0) isRPi = true;
        else if(strcmp(b.adapter, "S20") == 0) isRPi = false;
        else 
        {
            printf("i2c adapter: %s is invalid!!! (Options are \"RPi\" and \"S20\")\n", b.adapter);
        }
        b.s20 = new uHTRPowerMezzInterface(0, isV2, isRPi,b.hostname,b.port);

        if(isRPi && (b.adChan < 1 || b.adChan > 6)) 
        {
            printf("Invalid RPi adapter channel (1 - 6 are valid)!!!\n");
            continue;
        }
        else if(!isRPi)
        {
            b.adChan = -1; // unnecessary for s20 adapter
        }

        // Grab mezzanines 
        char slot[32];
        float voltage;
        bool mezzIn[]= {false,false,false,false,false};
        b.mezzanines = new Mezzanines(new boost::mutex());
        for(std::string mezz = parseto(config_line); mezz != ""; mezz = parseto(config_line))
        {
            if(sscanf(mezz.c_str(), "%s %f\n", slot, &voltage) == 2)
            {
                b.slots.push_back(std::make_pair(std::string(slot),voltage));

                if(strcmp(slot, "PM_3_3") == 0 && !mezzIn[0])
                {
                    b.mezzanines->push_back(new PM(*b.s20, V2_MUX_PM_3_3, voltage, true, b.adChan));
                    mezzIn[0]=true;
                }
                else if(strcmp(slot, "APM_2_5") == 0 && !mezzIn[1])
                {
                    b.mezzanines->push_back(new APM(*b.s20, V2_MUX_APM_2_5, voltage, true, b.adChan));
                    mezzIn[1]=true;
                }
                else if(strcmp(slot, "PM_1_B") == 0 && !mezzIn[2])
                {
                    b.mezzanines->push_back(new PM(*b.s20, V2_MUX_PM_1_B, voltage, true, b.adChan));
                    mezzIn[2]=true;
                }
                else if(strcmp(slot, "APM_1_6") == 0 && !mezzIn[3])
                {
                    b.mezzanines->push_back(new APM(*b.s20, V2_MUX_APM_1_6, voltage, true, b.adChan));
                    mezzIn[3]=true;
                }
                else if(strcmp(slot, "PM_1_A") == 0 && !mezzIn[4])
                {
                    b.mezzanines->push_back(new PM(*b.s20, V2_MUX_PM_1_A, voltage, true, b.adChan));
                    mezzIn[4]=true;
                }
                else
                {
                    printf("Invalid or duplicate Mezzanine slot: %s (options are PM_3_3, APM_2_5, PM_1_B, APM_1_6, PM_1_A)!!!\n", slot);
                }
            }
        }
        boards[b.id] = b;
    
    }
    initscr();
    if (has_colors())
    {
        start_color();
        /*if(can_change_color())
        {
            init_color(COLOR_RED, 500,0,0);
            init_color(COLOR_YELLOW, 926,675,54);
            init_pair(1, COLOR_RED,    COLOR_BLACK );
            init_pair(2, COLOR_YELLOW, COLOR_BLACK );
            init_pair(3, COLOR_BLACK,  COLOR_RED   );
            init_pair(4, COLOR_BLACK,  COLOR_YELLOW);
        }
        else
        {
            init_pair(1, COLOR_GREEN,    COLOR_BLACK );
            init_pair(2, COLOR_BLUE,     COLOR_BLACK );
            init_pair(3, COLOR_BLACK,    COLOR_GREEN );
            init_pair(4, COLOR_BLACK,    COLOR_BLUE  );
        }*/
    }
    cbreak(); 
    nodelay(stdscr, TRUE);
    noecho();
    erase();
}

int uHTRPowerMezzMenu::check_voltages(int id)
{
    uHTRPowerMezzInterface * s20 = boards[id].s20;

    if(s20->isV2_)
    {
        // Read voltage from sub20 board

        if(s20->isRPi_)
        {
            // 12 V supply is read through a divide by 9.2
            //
            if(9.2 * s20->readSUB20ADC(7 - 2*(boards[id].adChan - 1)) < 10.0) return RETVAL_BAD_SUPPLY_VOLTAGE;

            // 5 V supply is read through a dividr by 3.7
            if(3.7 * s20->readSUB20ADC(6 - 2*(boards[id].adChan - 1)) < 4.5) return RETVAL_BAD_3_3VOLT;

        }
        else
        {
            // 12 V supply is read through a divide by 9.2
            if(9.2 * s20->readSUB20ADC(6) < 10.0) return RETVAL_BAD_SUPPLY_VOLTAGE;

            // 5 V supply is read through a dividr by 3.7
            if(3.7 * s20->readSUB20ADC(7) < 4.5) return RETVAL_BAD_3_3VOLT;
        }
    }
    else
    {
        //--------------------------------------------------------------------
        // Set MUX to channel 1, AUX_POWER MODULE
        //--------------------------------------------------------------------
        // PCA9544A; SLAVE ADDRESS: 1110 000
        s20->setMUXChannel(MUX_AUXPOWERMOD, -1);

        //--------------------------------------------------------------------
        // Read 12V supply voltage. If 12V supply reads less than 8V then 
        // exit with a failure.
        //--------------------------------------------------------------------
        // MCP3428; SLAVE ADDRESS: 1101 010
        // Channel 3: V12 = 18.647 * V_CH3
        // 2.048V reference -> 1 mV per LSB
        if(18.647 * s20->readMezzADC(ADC_28, 3) < 10000.0) return RETVAL_BAD_SUPPLY_VOLTAGE;

        //--------------------------------------------------------------------
        // Read 3.3V supply voltage. If 3.3V supply reads less than 3V then 
        // exit with a failure.
        //--------------------------------------------------------------------
        // MCP3428; SLAVE ADDRESS: 1101 010
        // Channel 4: V3.3 = 18.647 * V_CH4
        // 2.048V reference -> 1 mV per LSB
        if(18.647 * s20->readMezzADC(ADC_28, 4) < 3000.0) return RETVAL_BAD_3_3VOLT;
    }
    return RETVAL_SUCCESS;
}

void uHTRPowerMezzMenu::display()
{
    attrset(A_REVERSE);

    for(int i = 1; i < COLS-1; i++)
    {
        mvaddch(0,i,'-');
        mvaddch(LINES-1,i,'-');
    }

    for(int i = 1; i < LINES-1; i++)
    {
        mvaddch(i,0,'|');
        mvaddch(i,COLS-1,'|');
    }
    mvaddch(0,0,'+');
    mvaddch(0,COLS-1,'+');
    mvaddch(LINES-1,0,'+');
    mvaddch(LINES-1,COLS-1,'+');
    refresh();

    attrset(A_NORMAL);


    char header1[256];
    char header2[256];
    char header3[256];
    char bars[256];
                   // 0         1         2         3         4         5         6         7         8         9         10        11        12        13
                   // 0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012
    sprintf(header1, " Board |          Mezz 1        |         Mezz 2         |         Mezz 3         |         Mezz 4         |         Mezz 5         |");
    sprintf(header2, "       |Temp(C) Volt(V) Power(W)|   Temp    Volt   Power |   Temp    Volt   Power |   Temp    Volt   Power |   Temp    Volt   Power |");
    sprintf(header3, "-------|------------------------|------------------------|------------------------|------------------------|------------------------|");
    sprintf(bars,    "       |                        |                        |                        |                        |                        |");
                   //"   88  |  12.34   12.34   12.34 |  12.34   12.34   12.34 |  12.34   12.34   12.34 |  12.34   12.34   12.34 |  12.34   12.34   12.34 |");
                  //         |1234567 1234567 1234567 | 
                  //         7         -25-           32                       57                       82                       107                      132 

    int yinit = 5;
    int xinit = 5;
    mvaddstr(yinit+1,xinit+1,header1);
    mvaddstr(yinit+2,xinit+1,header2);
    mvaddstr(yinit+3,xinit+1,header3);

    int x = xinit+1 , y = 4 + yinit;

    char buff[256];

    for(std::map<int,Board>::iterator board = boards.begin();
            board != boards.end();
            board++)
    {
        mvaddstr(y,x, bars);

        Board * b =  &board->second;
        //printf("Board %d",b->id); 
        sprintf(buff,"   %2d",b->id);
        mvaddstr(y,x,buff);
        x+=8;

        //check connection
        char response[32];
        bool fail = false;
        if (b->s20->can_connect())
        {
            int retval = check_voltages(b->id);
            char retmessage[32];
            Mezzanine::Summary::translateStatus(retval, retmessage);

            if(retval == RETVAL_SUCCESS)
            {
                b->mezzanines->monitor(true);
                Mezzanines::iterator iM = b->mezzanines->begin();
                for(;iM != b->mezzanines->end(); iM++)
                {
                    sprintf(buff,"%7.2f %7.2f %7.2f", (*iM)->actTest->temp[4], (*iM)->actTest->vout[4], (*iM)->actTest->P[4] );
                    //printf("mezz out: %7.2f %7.2f %7.2f\n", (*iM)->actTest->temp[4], (*iM)->actTest->vout[4], (*iM)->actTest->P[4] );

                    mvaddstr(y,x,buff);
                    x+=25;
                }
                sleep(1);
            }
            else
            {
                sprintf(response,"%s",retmessage);
                fail = true;
            }
        }
        else
        {
            sprintf(response,"Not Connected");
            fail = true;
        }

        if(fail) mvaddstr(y,x, response);
        y++;
        x=1+xinit;

        //printf("\n"); 
    }
    //move(LINES-3,2);
}

int uHTRPowerMezzMenu::start_test()
{

    int key_code;
    if(( key_code = getch()) == ERR)
        return 0;
    else
    {
        int board = key_code - '1';
        std::map<int, Board>::iterator b = boards.find(board);
        if(b != boards.end())
        {
            pid_t pid = fork();
            if(pid == 0) return board;
        }
        else if(key_code == 'q') return -1;
        return 0;
    }
}

void uHTRPowerMezzMenu::quit()
{
    for(std::map<int,Board>::iterator board = boards.begin();
            board != boards.end();
            board++)
    {
        //Board * b =  &board->second;
    }
    endwin();
    exit(0);
}
