#include "io.h"

io * io::m_ioInstance = NULL;

io::io()
{
    out = stdout;
    isCurses = false;
    m_x = 0;
    m_y = 0;
}
io * io::Instance()
{
    if (!m_ioInstance )
    {
        m_ioInstance = new io;
    }
    return m_ioInstance ;
}

void io::printf(const char * fmt, ...)
{
    io * p_io = io::Instance();
    va_list args;
    va_start(args,fmt);
    if(!p_io->isCurses)
    {
        vfprintf(p_io->out,fmt,args);
    }
    else
    {
        char buff[128];
        vsprintf(buff,fmt,args);
        mvaddstr(io::Instance()->m_y,io::Instance()->m_x,buff);
        refresh();
    }
    va_end(args);
}

void io::enable_curses(int y, int x)
{

    io::Instance()->isCurses = true;
    io::Instance()->m_y = y;
    io::Instance()->m_x = x;
}

void io::disable_curses()
{
    io::Instance()->isCurses = false;
}

