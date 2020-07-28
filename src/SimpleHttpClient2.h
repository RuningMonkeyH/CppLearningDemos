//
// Created by 孙宇 on 2020/7/24.
//
/***********************************************************************************************
 ***                                Y S H E N - S T U D I O S                                ***
 ***********************************************************************************************
                                                                                              
                  Project Name : CppLearningDemos 
                                                                                              
                     File Name : SimpleHttpClient2.h
                                                                                              
                    Programmer : 孙宇 
                                                                                              
                    Start Date : 2020/7/24 
                                                                                              
                   Last Update : 2020/7/24 
                                                                                              
 *---------------------------------------------------------------------------------------------*
  Description:           
        异步http请求客户端示例。1.73版本
        主要区别在于：
            ioc使用make_strand包装
            使用了tcp_stream进行连接
            使用了beast内部handler，1.69使用的std::bind
            新版本封装度更高，使用方便些，但要去了解实现
  
 *---------------------------------------------------------------------------------------------*
  Functions:
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CPPLEARNINGDEMOS_SIMPLEHTTPCLIENT2_H
#define CPPLEARNINGDEMOS_SIMPLEHTTPCLIENT2_H

#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <string>
#include <functional>
#include <memory>

namespace http = boost::beast::http;
using tcp = boost::asio::ip::tcp;

class SimpleHttpClient2 : public std::enable_shared_from_this<SimpleHttpClient2> {
private:
    tcp::resolver resolver_;
    boost::beast::tcp_stream stream_;
    boost::beast::flat_buffer buffer_;
    http::request<http::empty_body> req_;
    http::response<http::string_body> res_;

    static void fail(boost::beast::error_code ec, char const *what) {
        std::cerr << what << ": " << ec.message() << std::endl;
    }

public:
    explicit SimpleHttpClient2(boost::asio::io_context &ioc)
            : resolver_(boost::asio::make_strand(ioc)), stream_(boost::asio::make_strand(ioc)) {
        //构造函数
    }

    void run(char const *host,
             char const *port,
             char const *target,
             int version) {
        req_.version(version);
        req_.method(http::verb::get);
        req_.target(target);
        req_.set(http::field::host, host);
        req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        resolver_.async_resolve(
                host,
                port,
                boost::beast::bind_front_handler(
                        &SimpleHttpClient2::on_resolve,
                        shared_from_this()));
    }

    void on_resolve(boost::beast::error_code ec,
                    tcp::resolver::results_type results) {
        if (ec)
            return fail(ec, "on_resolve");

        stream_.expires_after(std::chrono::seconds(30));//设置超时时间

        stream_.async_connect(results,
                              boost::beast::bind_front_handler(
                                      &SimpleHttpClient2::on_connect,
                                      shared_from_this()));

    }

    void on_connect(boost::beast::error_code ec,
                    tcp::resolver::results_type::endpoint_type endpoint) {
        if (ec)
            return fail(ec, "on_connect");
        stream_.expires_after(std::chrono::seconds(30));

        http::async_write(stream_,
                          req_,
                          boost::beast::bind_front_handler(
                                  &SimpleHttpClient2::on_write,
                                  shared_from_this()));
    }

    void on_write(boost::beast::error_code ec,
                  std::size_t bytes_transferred) {
        if (ec)
            return fail(ec, "on_write");

        http::async_read(stream_,
                         buffer_,
                         res_,
                         boost::beast::bind_front_handler(
                                 &SimpleHttpClient2::on_read,
                                 shared_from_this()));

    }

    void on_read(boost::beast::error_code ec,
                 std::size_t bytes_transferred) {
        if (ec)
            return fail(ec, "on_read");

        std::cout << res_ << std::endl;

        stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

        if (ec && ec != boost::beast::errc::not_connected)
            return fail(ec, "shutdown");
    }

};


#endif //CPPLEARNINGDEMOS_SIMPLEHTTPCLIENT2_H
