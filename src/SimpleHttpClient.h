//
// Created by 孙宇 on 2020/7/24.
//
/***********************************************************************************************
 ***                                Y S H E N - S T U D I O S                                ***
 ***********************************************************************************************
                                                                                              
                  Project Name : CppLearningDemos 
                                                                                              
                     File Name : SimpleHttpClient.h 
                                                                                              
                    Programmer : 孙宇 
                                                                                              
                    Start Date : 2020/7/24 
                                                                                              
                   Last Update : 2020/7/24 
                                                                                              
 *---------------------------------------------------------------------------------------------*
  Description:
        一个简单的Get请求客户端示例。 1.69版本

 *---------------------------------------------------------------------------------------------*
  Functions:
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CPPLEARNINGDEMOS_SIMPLEHTTPCLIENT_H
#define CPPLEARNINGDEMOS_SIMPLEHTTPCLIENT_H

#include <iostream>
#include <boost/timer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <string>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

class SimpleHttpClient : public std::enable_shared_from_this<SimpleHttpClient> {

private:
    //各成员
    tcp::resolver resolver_;
    tcp::socket socket_;
    boost::beast::flat_buffer buffer_;
    http::request<http::empty_body> req_;
    http::response<http::string_body> res_;

public:
    //构造函数
    explicit SimpleHttpClient(boost::asio::io_context &ioc)
            : resolver_(ioc), socket_(ioc) {
    }

    //线程起步
    void start(char const *host,
             char const *port,
             char const *target,
             int version) {
        //设置请求体信息
        req_.version(version);
        req_.method(http::verb::get);
        req_.target(target);
        req_.set(http::field::host, host);
        req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        //异步处理
        resolver_.async_resolve(
                host,
                port,
                std::bind(
                        &SimpleHttpClient::on_resolve,
                        shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2));
    }

    //解析
    void on_resolve(boost::system::error_code ec,
                    const tcp::resolver::results_type& results) {
        if (ec)
            return fail(ec, "resolve");

        //异步连接
        boost::asio::async_connect(
                socket_,
                results.begin(),
                results.end(),
                std::bind(
                        &SimpleHttpClient::on_connect,
                        shared_from_this(),
                        std::placeholders::_1));
    }

    //建立连接
    void on_connect(boost::system::error_code ec) {
        if (ec)
            return fail(ec, "connect");
        http::async_write(
                socket_,
                req_,
                std::bind(
                        &SimpleHttpClient::on_write,
                        shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2));
    }

    //发请求
    void on_write(boost::system::error_code ec,
                  std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec)
            return fail(ec, "write");
        //收响应
        http::async_read(
                socket_,
                buffer_,
                res_,
                std::bind(
                        &SimpleHttpClient::on_read,
                        shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2));
    }

    //读结果
    void on_read(boost::system::error_code ec,
                 std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec)
            return fail(ec, "read");
        //输出请求结果
        std::cout << res_ << std::endl;
        //关闭连接
        socket_.shutdown(tcp::socket::shutdown_both, ec);

        if (ec && ec != boost::system::errc::not_connected)
            return fail(ec, "shutdown");
    }

    static void fail(boost::system::error_code ec, char const *what) {
        std::cerr << what << ":" << ec.message() << std::endl;
    }

};


#endif //CPPLEARNINGDEMOS_SIMPLEHTTPCLIENT_H
