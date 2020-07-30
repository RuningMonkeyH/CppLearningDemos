//
// Created by 孙宇 on 2020/7/30.
//
/***********************************************************************************************
 ***                                Y S H E N - S T U D I O S                                ***
 ***********************************************************************************************
                                                                                              
                  Project Name : CppLearningDemos 
                                                                                              
                     File Name : SimpleHttpServer.h 
                                                                                              
                    Programmer : 孙宇 
                                                                                              
                    Start Date : 2020/7/30 
                                                                                              
                   Last Update : 2020/7/30 
                                                                                              
 *---------------------------------------------------------------------------------------------*
  Description:           
        简易异步Http服务端示例
  
 *---------------------------------------------------------------------------------------------*
  Functions:
 *---------------------------------------------------------------------------------------------*
  Updates:
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CPPLEARNINGDEMOS_SIMPLEHTTPSERVER_H
#define CPPLEARNINGDEMOS_SIMPLEHTTPSERVER_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include "RequestHandler.h"
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class SimpleHttpServer : public std::enable_shared_from_this<SimpleHttpServer> {
private:

public:

};


#endif //CPPLEARNINGDEMOS_SIMPLEHTTPSERVER_H
