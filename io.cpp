#include "io.h"
#include <string.h>

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
        attrset(COLOR_PAIR(2));
        char buff[128];
        vsprintf(buff,fmt,args);
        if(strlen(buff) > p_io->m_long) p_io->m_long =strlen(buff);

        mvaddstr(p_io->m_y1 + p_io->m_i , p_io->m_x,buff);
        refresh();
        p_io->inc_y();
    }
    va_end(args);
}

void io::enable_curses(int y1, int y2, int x)
{
    io * p = Instance();
    p->isCurses = true;
    p->m_y1 = y1;
    p->m_y2 = y2;
    p->m_x = x;
    p->m_long = 0;
    p->m_wait = false;

}

void io::set_wait(bool wait)
{
    io * p = Instance();
    if(!p->isCurses || p->m_wait == wait) return;
    p->m_wait = wait;

    if(wait)
        nodelay(stdscr, false);
    else
        nodelay(stdscr, false);
}

void io::disable_curses()
{
    io::Instance()->isCurses = false;
}

void io::inc_y(int i)
{
    if(m_i + i >= m_y2 - m_y1) 
    {
        char buff[128];
        if(m_wait)
        {
            sprintf(buff,"Press a key to continue");
            mvaddstr(m_y2,m_x,buff);
            getch();
        }
        sprintf(buff,"%*s",m_long," ");
        for(int j = m_y1; j <= m_y2; ++j) mvaddstr(j,m_x,buff);
        m_long = 0;
    }

    m_i = ( m_i + i ) % ( m_y2 - m_y1 );
}
