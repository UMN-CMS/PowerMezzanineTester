#include <stdio.h>
#include <stdarg.h>
#include <curses.h>

class io
{
    public:
        static io * Instance();
        FILE * out;

        static void printf(const char * fmt, ...);
        static void enable_curses(int y1, int y2, int x);
        static void disable_curses() ;

        bool isCurses;
        int m_y1;
        int m_y2;
        int m_x;
        int m_i;
    private:
        void inc_y(int i = 1);
        io();
        static io * m_ioInstance;

};

