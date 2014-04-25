#include "io.h"

io * io::m_ioInstance = NULL;

io * io::Instance()
{
    if (!m_ioInstance )
    {
        m_ioInstance = new io;
    }
    return m_ioInstance ;
}

void io::printf(char str[])
{
    io * p_io = io::Instance();
    fprintf(p_io->out, str);
}

