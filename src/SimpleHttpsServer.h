//
// Created by 孙宇 on 2020/7/31.
//
/***********************************************************************************************
 ***                                Y S H E N - S T U D I O S                                ***
 ***********************************************************************************************
                                                                                              
                  Project Name : CppLearningDemos 
                                                                                              
                     File Name : SimpleHttpsServer.h 
                                                                                              
                    Programmer : 孙宇 
                                                                                              
                    Start Date : 2020/7/31 
                                                                                              
                   Last Update : 2020/7/31 
                                                                                              
 *---------------------------------------------------------------------------------------------*
  Description:           
        异步https服务端示例
  
 *---------------------------------------------------------------------------------------------*
  Functions:
 *---------------------------------------------------------------------------------------------*
  Updates:
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CPPLEARNINGDEMOS_SIMPLEHTTPSSERVER_H
#define CPPLEARNINGDEMOS_SIMPLEHTTPSSERVER_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "RequestHandler.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
/**
 * 连续写的http和https两个类
 * 代码除了需要包装下tcp流和握手之外几乎一样
 * 测试的时候https这里一直出现shared_from_this弱指针报错
 * 并且错误栈中报的是指向了http那个类的shared指针，这就很奇怪
 * 折腾了半天，查到一个原因
 * shared_from_this()是enable_shared_from_this<T>的成员函数，返回shared_ptr<T>;
 * 注意的是，这个函数仅在shared_ptr<T>的构造函数被调用之后才能使用。
 * 原因是enable_shared_from_this::weak_ptr并不在构造函数中设置，而是在shared_ptr<T>的构造函数中设置。
 * 所以为什么没调用构造函数呢，在写继承的时候忘记加public了。晕了
 */
class SimpleHttpsServer : public std::enable_shared_from_this<SimpleHttpsServer> {
private:
    struct send_lambda {
        SimpleHttpsServer &self_;

        explicit send_lambda(SimpleHttpsServer &self)
                : self_(self) {}

        template<bool isRequest, class Body, class Fields>
        void operator()(http::message<isRequest, Body, Fields> &&msg) const {
            //msg生命周期必须持续在异步操作中，所以用一个shared_ptr来管理
            auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(std::move(msg));
            self_.res_ = sp;
            http::async_write(
                    self_.stream_,
                    *sp,
                    beast::bind_front_handler(
                            &SimpleHttpsServer::on_write,
                            self_.shared_from_this(),
                            sp->need_eof()));
        }
    };

    beast::ssl_stream<beast::tcp_stream> stream_; //前部分与http服务只相差这一个流包装
    beast::flat_buffer buffer_;
    std::shared_ptr<std::string const> doc_root_;
    http::request<http::string_body> req_;
    std::shared_ptr<void> res_;
    send_lambda lambda_;

public:
    explicit SimpleHttpsServer(
            tcp::socket &&socket,
            ssl::context &ctx,
            std::shared_ptr<std::string const> doc_root
    ) : stream_(std::move(socket), ctx),
        doc_root_(std::move(doc_root)),
        lambda_(*this) {}

    void run() {
        net::dispatch(stream_.get_executor(),
                      beast::bind_front_handler(
                              &SimpleHttpsServer::on_run,
                              shared_from_this()));
    }

    void on_run() {
        beast::get_lowest_layer(stream_)
                .expires_after(std::chrono::seconds(30));
        //与http不同的是在开始读请求前先握手
        stream_.async_handshake(
                ssl::stream_base::server,
                beast::bind_front_handler(
                        &SimpleHttpsServer::on_handshake,
                        shared_from_this()));
    }

    void on_handshake(beast::error_code ec) {
        if (ec)
            return fail(ec, "on_handshake");
        do_read();
    }

    void do_read() {
        //后续步骤与http相同
        req_ = {};
        beast::get_lowest_layer(stream_)
                .expires_after(std::chrono::seconds(30));
        http::async_read(stream_, buffer_, req_,
                         beast::bind_front_handler(
                                 &SimpleHttpsServer::on_read,
                                 shared_from_this()));
    }

    void on_read(beast::error_code ec,
                 size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec == http::error::end_of_stream)
            return do_close();
        if (ec)
            return fail(ec, "on_read");
        handle_request(*doc_root_, std::move(req_), lambda_);
    }

    void on_write(bool close,
                  beast::error_code ec,
                  size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec)
            return fail(ec, "on_write");
        if (close) {
            return do_close();
        }
        res_ = nullptr;
        do_read();
    }

    void do_close() {
        beast::get_lowest_layer(stream_)
                .expires_after(std::chrono::seconds(30));
        stream_.async_shutdown(
                beast::bind_front_handler(
                        &SimpleHttpsServer::on_shutdown,
                        shared_from_this()));
    }

    void on_shutdown(beast::error_code ec) {
        if (ec)
            return fail(ec, "on_shutdown");
    }
};

class SimpleHttpsServerListener : public std::enable_shared_from_this<SimpleHttpsServerListener> {
private:
    net::io_context &ioc_;
    ssl::context &ctx_;
    tcp::acceptor acceptor_;
    std::shared_ptr<std::string const> doc_root_;
public:
    SimpleHttpsServerListener(
            net::io_context &ioc,
            ssl::context &ctx,
            tcp::endpoint endpoint,
            std::shared_ptr<std::string const> doc_root
    ) : ioc_(ioc),
        ctx_(ctx),
        acceptor_(ioc),
        doc_root_(std::move(doc_root)) {
        beast::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            fail(ec, "open");
            return;
        }
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) {
            fail(ec, "set_option");
            return;
        }
        acceptor_.bind(endpoint, ec);
        if (ec) {
            fail(ec, "bind");
            return;
        }
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec) {
            fail(ec, "listen");
            return;
        }
    }

    void run() {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
                net::make_strand(ioc_),
                beast::bind_front_handler(
                        &SimpleHttpsServerListener::on_accept,
                        shared_from_this()));
    }

    void on_accept(beast::error_code ec,
                   tcp::socket socket) {
        if (ec) {
            fail(ec, "accept");
        } else {
            std::make_shared<SimpleHttpsServer>(
                    std::move(socket),
                    ctx_,
                    doc_root_
            )->run();
        }
        do_accept();
    }
};


#endif //CPPLEARNINGDEMOS_SIMPLEHTTPSSERVER_H
