//
// Created by 孙宇 on 2020/7/25.
//
/***********************************************************************************************
 ***                                Y S H E N - S T U D I O S                                ***
 ***********************************************************************************************
                                                                                              
                  Project Name : CppLearningDemos 
                                                                                              
                     File Name : SimpleHttpsClient.h
                                                                                              
                    Programmer : 孙宇 
                                                                                              
                    Start Date : 2020/7/25
                                                                                              
                   Last Update : 2020/7/25
                                                                                              
 *---------------------------------------------------------------------------------------------*
  Description:           
        异步https请求示例，基于boost 1.7.3
  
 *---------------------------------------------------------------------------------------------*
  Functions:
 *---------------------------------------------------------------------------------------------*
  Updates:
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CPPLEARNINGDEMOS_SIMPLEHTTPSCLIENT_H
#define CPPLEARNINGDEMOS_SIMPLEHTTPSCLIENT_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace http = boost::beast::http;           // from <boost/beast/http.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class SimpleHttpsClient : public std::enable_shared_from_this<SimpleHttpsClient> {
private:
    tcp::resolver resolver_;
    boost::beast::ssl_stream<boost::beast::tcp_stream> stream_;
    boost::beast::flat_buffer buffer_;
    http::request<http::empty_body> req_;
    http::response<http::string_body> res_;

    static void fail(boost::beast::error_code ec, char const *what) {
        std::cerr << what << ": " << ec.message() << std::endl;
    }

public:
    explicit SimpleHttpsClient(
            boost::asio::executor ex,
            ssl::context &ctx)
            : resolver_(ex), stream_(ex, ctx) {}

    void run(char const *host,
             char const *port,
             char const *target,
             int version) {
        //设置SNI，很多主机可能需要这一步握手成功
        if (!SSL_set_tlsext_host_name(stream_.native_handle(), host)) {
            boost::beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
            std::cerr << ec.message() << "\n";
            return;
        }
        //设置请求体
        req_.version(version);
        req_.method(http::verb::get);
        req_.target(target);
        req_.set(http::field::host, host);
        req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        //解析域名
        resolver_.async_resolve(
                host,
                port,
                boost::beast::bind_front_handler(
                        &SimpleHttpsClient::on_resolve,
                        shared_from_this()));

    }

    void on_resolve(boost::beast::error_code ec,
                    tcp::resolver::results_type results) {
        if (ec)
            return fail(ec, "on_resolve");
        //拿到解析结果，准备连接
        boost::beast::get_lowest_layer(stream_)
                .expires_after(std::chrono::seconds(10));//超时
        boost::beast::get_lowest_layer(stream_)
                .async_connect(
                        results,
                        boost::beast::bind_front_handler(
                                &SimpleHttpsClient::on_connect,
                                shared_from_this()));
    }

    void on_connect(boost::beast::error_code ec,
                    tcp::resolver::results_type::endpoint_type endpoint) {
        if (ec)
            return fail(ec, "on_connect");
        //连通后握手
        stream_.async_handshake(
                ssl::stream_base::client,
                boost::beast::bind_front_handler(
                        &SimpleHttpsClient::on_handshake,
                        shared_from_this()));
    }

    void on_handshake(boost::beast::error_code ec) {
        if (ec)
            return fail(ec, "on_handshake");
        boost::beast::get_lowest_layer(stream_)
                .expires_after(std::chrono::seconds(30));//超时
        //发送请求
        http::async_write(
                stream_,
                req_,
                boost::beast::bind_front_handler(
                        &SimpleHttpsClient::on_write,
                        shared_from_this()));
    }

    void on_write(boost::beast::error_code ec,
                  std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec)
            return fail(ec, "on_write");
        //读取响应
        http::async_read(
                stream_,
                buffer_,
                res_,
                boost::beast::bind_front_handler(
                        &SimpleHttpsClient::on_read,
                        shared_from_this()));
    }

    void on_read(boost::beast::error_code ec,
                 size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec)
            return fail(ec, "on_read");
        //打印结果
        std::cout << res_ << std::endl;
        boost::beast::get_lowest_layer(stream_)
                .expires_after(std::chrono::seconds(30));
        //关闭连接，回收资源
        stream_.async_shutdown(
                boost::beast::bind_front_handler(
                        &SimpleHttpsClient::on_shutdown,
                        shared_from_this()));
    }

    void on_shutdown(boost::beast::error_code ec) {
        if (ec == boost::asio::error::eof) {
            ec = {};
        }
        if (ec)
            return fail(ec, "on_shutdown");
    }
};


#endif //CPPLEARNINGDEMOS_SIMPLEHTTPSCLIENT_H
