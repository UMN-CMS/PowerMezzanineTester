#include <stdio.h>
#include <stdarg.h>
#include <curses.h>

class io
{
    public:
        static io * Instance();
        FILE * out;

        static void printf(const char * fmt, ...);
        static void enable_curses(int y, int x);
        static void disable_curses() ;

        bool isCurses;
        int m_y;
        int m_x;
    private:
        io();
        static io * m_ioInstance;

};

