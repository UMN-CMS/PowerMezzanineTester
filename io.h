#include <stdio.h>

class io
{
    public:
        static io * Instance();
        FILE * out;
        static void printf(char str[]);
        // 1 argument
        template<typename A>
        static void printf(char str[], A a)
        {
            io * p_io = io::Instance();
            fprintf(p_io->out, str, a);
        }
        // 2 argument
        template<typename A,typename B>
        static void printf(char str[], A a, B b)
        {
            io * p_io = io::Instance();
            fprintf(p_io->out, str, a, b);
        }
        // 3 argument
        template<typename A,typename B, typename C>
        static void printf(char str[], A a, B b, C c)
        {
            io * p_io = io::Instance();
            fprintf(p_io->out, str, a, b, c);
        }
        // 4 argument
        template<typename A,typename B, typename C, typename D>
        static void printf(char str[], A a, B b, C c, D d)
        {
            io * p_io = io::Instance();
            fprintf(p_io->out, str, a, b, c, d);
        }
        // 5 argument
        template<typename A,typename B, typename C, typename D, typename E>
        static void printf(char str[], A a, B b, C c, D d, E e)
        {
            io * p_io = io::Instance();
            fprintf(p_io->out, str, a, b, c, d, e);
        }
        // 6 argument
        template<typename A,typename B, typename C, typename D, typename E, typename F>
        static void printf(char str[], A a, B b, C c, D d, E e, F f)
        {
            io * p_io = io::Instance();
            fprintf(p_io->out, str, a, b, c, d, e, f);
        }
    private:
        io(){out = stdout;}
        static io * m_ioInstance;
};

