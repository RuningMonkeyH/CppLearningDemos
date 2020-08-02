//
// Created by 孙宇 on 2020/8/3.
//
/***********************************************************************************************
 ***                                Y S H E N - S T U D I O S                                ***
 ***********************************************************************************************
                                                                                              
                  Project Name : CppLearningDemos 
                                                                                              
                     File Name : forwarder.cpp 
                                                                                              
                    Programmer : 孙宇 
                                                                                              
                    Start Date : 2020/8/3 
                                                                                              
                   Last Update : 2020/8/3 
                                                                                              
 *---------------------------------------------------------------------------------------------*
  Description:           
        准备写个简单的转发器
  
 *---------------------------------------------------------------------------------------------*
  Functions:
 *---------------------------------------------------------------------------------------------*
  Updates:
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <iostream>
#include <boost/thread/xtime.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/config.hpp>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

int main() {
    //计时
    auto start_time = boost::get_system_time();
    std::cout << "Ready, GO!" << std::endl << std::endl;



    long end_time = (boost::get_system_time() - start_time).total_milliseconds();
    std::cout << "Completed in: " << end_time << "ms" << std::endl;
    return 0;
}