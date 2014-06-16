#ifndef UHTR_POWERMEZZ_MENU_INTERFACE
#define UHTR_POWERMEZZ_MENU_INTERFACE

#include "uHTRPowerMezzInterface.h"
#include "uHTRMezzanines.h"
#include <string>
#include <vector>
#include <map>

class uHTRPowerMezzMenu
{
    struct Board
    {
        int id;
        int adChan;
        int pid;
        int time;
        bool isConnected;
        int voltage;
        char adapter[4];
        char hostname[32];
        char port[8];
        std::vector<std::pair<std::string,float> > slots;
        uHTRPowerMezzInterface * s20;
        Mezzanines * mezzanines;
    };

    public:
        uHTRPowerMezzMenu(std::map< int, std::string> config_lines, bool isV2, const char tester[] = "");
        void display();
        void query_servers();
        void query_server(Board * b);
        void quit();
        int start_test(char *responce = 0);

    private:
        void check_voltages();
        void draw_star();
        char readch();
        void readstr(char str[]);
        std::map<int, Board>::iterator  selected_board_;
        std::map<int, Board> boards_;
        const int yinit_;
        const int xinit_;
        char tester[64];
        
};

#endif
