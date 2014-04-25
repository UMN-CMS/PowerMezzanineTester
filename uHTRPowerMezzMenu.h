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
        char adapter[4];
        char hostname[32];
        char port[8];
        std::vector<std::pair<std::string,float> > slots;
        uHTRPowerMezzInterface * s20;
        Mezzanines * mezzanines;
    };
    public:
        uHTRPowerMezzMenu(std::map< int, std::string> config_lines, bool isV2, bool ncurses);
        void display();
        void quit();
        int start_test();
        int check_voltages(int i);

    private:
        std::map<int, Board> boards;
        int loops_;
        bool ncurses_;
        
};

#endif
