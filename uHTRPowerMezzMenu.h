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
        char adapter[4];
        char hostname[32];
        char port[8];
        std::vector<std::pair<std::string,float> > slots;
        uHTRPowerMezzInterface * s20;
        Mezzanines * mezzanines;
    };
    enum MenuMode
    {
        NORMAL,
        STARTING,
        STOPPING,
        IN_TESTER
    };


    public:
        uHTRPowerMezzMenu(std::map< int, std::string> config_lines, bool isV2, bool ncurses);
        void display();
        void query_servers();
        void quit();
        int start_test();

    private:
        int check_voltages(int i);
        void draw_star();
        MenuMode mmode_;
        std::map<int, Board>::iterator  selected_board_;
        std::map<int, Board> boards_;
        const int yinit_;
        const int xinit_;
        
};

#endif
