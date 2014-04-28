#include "io.h"

io * io::m_ioInstance = NULL;

io::io()
{
    out = stdout;
    isCurses = false;
    m_x = 0;
    m_y1 = 0;
    m_y2 = 0;
    m_i = 0;
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
        mvaddstr(io::Instance()->m_y1 + io::Instance()->m_i , io::Instance()->m_x,buff);
        refresh();
        io::Instance()->inc_y();
    }
    va_end(args);
}

void io::enable_curses(int y1, int y2, int x)
{

    io::Instance()->isCurses = true;
    io::Instance()->m_y1 = y1;
    io::Instance()->m_y2 = y2;
    io::Instance()->m_x = x;
}

void io::disable_curses()
{
    io::Instance()->isCurses = false;
}

void io::inc_y(int i)
{
    m_i = ( m_i + i ) % ( m_y2 - m_y1 );
}
