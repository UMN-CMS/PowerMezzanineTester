//
// blocking_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "comInterfaceServer.h"

#define V2_I2C_SADDRESS_RPI_MUX    0x77
#define V2_I2C_SADDRESS_BASE_MUX    0x70

using boost::asio::ip::tcp;

const int max_length = 1024;

enum Mode {
    READ = 1,
    WRITE = 2,
    DISPLAY = 3,
    START = 4,
    STOP = 5,
    TIME = 6
};

typedef tcp::socket*  socket_ptr;


void session(socket_ptr sock, RPiInterfaceServer& rpi )
{
    try
    {
        //read mode first
        std::vector<int> header(5);
        boost::asio::read(*sock, boost::asio::buffer(header,5*sizeof(int)));
        int address = header.at(0);
        Mode mode = Mode(header.at(1));
        int length = header.at(2);
        int adChan = header.at(3);
        int bbChan = header.at(4);

        printf("header: %x,%i,%i,%i,%i \n",address,mode,length,adChan,bbChan);
        int error = 0;
        if (mode == READ || mode == WRITE)
        {
            //Set RPI Mux 
            char buff = (char)(1 << adChan);
            error |= rpi.i2c_write(V2_I2C_SADDRESS_RPI_MUX, &buff, 1);

            //set board mux
            buff = (char)(1 << bbChan);
            error |= rpi.i2c_write(V2_I2C_SADDRESS_BASE_MUX, &buff, 1);
        }

        //switch over mode
        char data[max_length];
        int ret[2];
        std::string response;
        switch (mode)
        {
            case WRITE:
                boost::asio::read(*sock, boost::asio::buffer(data,length));
                error |= rpi.i2c_write(address,data,length);

                break;
            case READ:
                error |= rpi.i2c_read(address,data,length);
                boost::asio::write(*sock, boost::asio::buffer(data,length));
                break;
            case DISPLAY:
                boost::asio::read(*sock, boost::asio::buffer(data,length));
                rpi.lcd_write(data,length);
                break;
            case START:
                rpi.startTest(adChan,address);
                break;
            case STOP:
                rpi.stopTest(adChan);
                break;
            case TIME:
                rpi.readTest(adChan,ret);
                boost::asio::write(*sock, boost::asio::buffer(ret,length));
                break;
            default:
                std::cerr << "invalid mode\n";
                break;
        }
        boost::asio::write(*sock, boost::asio::buffer(&error,sizeof(error)));
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }
}

void server(boost::asio::io_service& io_service, short port)
{
    RPiInterfaceServer rpi;
    tcp::acceptor a(io_service, tcp::endpoint(tcp::v4(), port));
    for (;;)
    {
        socket_ptr sock(new tcp::socket(io_service));
        a.accept(*sock);
        session(sock,rpi);
        delete sock;
    }
}

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: server <port>\n";
      return 1;
    }

    boost::asio::io_service io_service;

    using namespace std; // For atoi.
    server(io_service, atoi(argv[1]));
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
