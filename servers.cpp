//
// Created by 孙宇 on 2020/7/28.
//
/***********************************************************************************************
 ***                                Y S H E N - S T U D I O S                                ***
 ***********************************************************************************************
                                                                                              
                  Project Name : CppLearningDemos 
                                                                                              
                     File Name : servers 
                                                                                              
                    Programmer : 孙宇 
                                                                                              
                    Start Date : 2020/7/28 
                                                                                              
                   Last Update : 2020/7/30
                                                                                              
 *---------------------------------------------------------------------------------------------*
  Description:           
        各Servers示例学习
  
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
#include <boost/config.hpp>
#include "src/SimpleHttpServer.h"
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

void syncHttpServer();

void syncHttpsServer();

void asyncHttpServer();

auto const address = "0.0.0.0";
auto const port = 8080;//端口一般用unsigned short
auto const ssl_port = 445;
auto const root = ".";//要把工作目录配置到当前项目目录才能访问项目根目录下写的页面

int main() {
    //计时
    auto start_time = boost::get_system_time();
    std::cout << "Ready, GO!" << std::endl << std::endl;

    syncHttpsServer();

    long end_time = (boost::get_system_time() - start_time).total_milliseconds();
    std::cout << "Completed in: " << end_time << "ms" << std::endl;
    return 0;
}

//发包结构体,重写操作符很巧妙
template<class Stream>
struct send_lambda {
    Stream &stream_;
    bool &close_;
    beast::error_code &ec_;

    explicit send_lambda(
            Stream &stream,
            bool &close,
            beast::error_code &ec
    ) : stream_(stream),
        close_(close),
        ec_(ec) {}

    //重写操作符
    template<bool isRequest, class Body, class Fields>
    void operator()(http::message<isRequest, Body, Fields> &&msg) const {
        close_ = msg.need_eof();
        // We need the serializer here because the serializer requires
        // a non-const file_body, and the message oriented version of
        // http::write only works with const messages.
        http::serializer<isRequest, Body, Fields> sr{msg};
        http::write(stream_, sr, ec_);
    }
};

//处理连接
void do_session(
        tcp::socket &socket,
        std::shared_ptr<std::string const> const &doc_root
) {
    bool close = false;
    beast::error_code ec;

    beast::flat_buffer buffer;
    //初始化发包体
    send_lambda<tcp::socket> lambda{socket, close, ec};
    for (;;) {
        //读请求
        http::request<http::string_body> req;
        http::read(socket, buffer, req, ec);
        if (ec == http::error::end_of_stream)
            break;
        if (ec)
            return fail(ec, "read");
        //回复响应，Todo 这句知识点有点多呀，好好理解下
        handle_request(*doc_root, std::move(req), lambda);
        if (ec)
            return fail(ec, "write");
        if (close) {
            break;
        }
    }
    //关闭请求
    socket.shutdown(tcp::socket::shutdown_send, ec);
    if (ec)
        return fail(ec, "shutdown");

}

void syncHttpServer() {
    try {
        net::io_context ioc{1};
        //请求接收器
        auto address_ = net::ip::make_address(address);
        auto port_ = static_cast<unsigned short>(port);
        auto root_ = std::make_shared<std::string>(root);
        tcp::acceptor acceptor{ioc, {address_, port_}};
        for (;;) {
            tcp::socket socket{ioc};
            //断点看这里是个阻塞，只有接收到请求才会继续进行下一步
            acceptor.accept(socket);
            std::thread(std::bind(
                    &do_session,
                    std::move(socket),
                    root_
            )).detach();
        }
    }
    catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

//处理ssl连接,与http基本只区别于流载体和握手
void do_ssl_session(
        tcp::socket &socket,
        ssl::context &ctx,
        std::shared_ptr<std::string const> const &doc_root
) {
    bool close = false;
    beast::error_code ec;
    //封装tcp流为ssl tcp流
    beast::ssl_stream<tcp::socket &> stream{socket, ctx};
    //握手
    stream.handshake(ssl::stream_base::server, ec);
    if (ec)
        return fail(ec, "handshake");
    //其余步骤与http基本一致
    beast::flat_buffer buffer;
    send_lambda<beast::ssl_stream<tcp::socket &>> lambda{stream, close, ec};
    for (;;) {
        http::request<http::string_body> req;
        http::read(stream, buffer, req, ec);
        if (ec == http::error::end_of_stream)
            break;
        if (ec)
            return fail(ec, "read");
        handle_request(*doc_root, std::move(req), lambda);
        if (ec)
            return fail(ec, "write");//这里ec是通过lambda递交的
        if (close) {
            break;
        }
        stream.shutdown(ec);
        if (ec)
            return fail(ec, "shutdown");
    }
}

//同步https服务
void syncHttpsServer() {
    try {
        auto address_ = net::ip::make_address(address);
        auto port_ = static_cast<unsigned short>(ssl_port);
        auto root_ = std::make_shared<std::string>(root);

        net::io_context ioc{1};
        ssl::context ctx(ssl::context::sslv23);
//        load_server_cerificate(ctx);//官方示例
        //自己的证书测试
        ctx.use_certificate_chain_file("crt.crt");
        ctx.use_private_key_file("key.key", ssl::context::file_format::pem);
        //接收器
        tcp::acceptor acceptor{ioc, {address_, port_}};
        for (;;) {
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            std::thread(std::bind(
                    &do_ssl_session,
                    std::move(socket),
                    /**
                     * 为什么C++11又引入了std::ref？
                     * 主要是考虑函数式编程（如std::bind）在使用时，是对参数直接拷贝，而不是引用
                     * 可能有一些需求，使得C++11的设计者认为默认应该采用拷贝，如果使用者有需求，加上std::ref即可
                     */
                    std::ref(ctx),
                    root_
            )).detach();
        }
    }
    catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

}

//异步http服务端
void asyncHttpServer() {

}