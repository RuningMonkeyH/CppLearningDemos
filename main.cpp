#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <string>
#include "src/SimpleHttpClient.h"
#include "src/SimpleHttpClient2.h"
#include "src/ROOT_CERITICATES.h"
#include <boost/thread/xtime.hpp>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

auto const host = "www.boost.org";
auto const ssl_host = "www.digicert.com";
auto const port = "80";
auto const ssl_port = "443";
auto const target = "/doc/libs/1_73_0/libs/beast/doc/html/index.html";
auto const ssl_target = "/";
int version = 11;

void syncHttpRequest(int num);

void asyncHttpRequest(int num);

void syncHttpsRequest();

int main() {
    //计时
    auto start_time = boost::get_system_time();
    std::cout << "Ready, GO!" << std::endl << std::endl;

    for (int i = 0; i < 1; ++i) {
//        syncHttpRequest(i);
//        asyncHttpRequest(i);
        syncHttpsRequest();
    }

    long end_time = (boost::get_system_time() - start_time).total_milliseconds();
    std::cout << "Completed in: " << end_time << "ms" << std::endl;
    return 0;
}

/*---------------------------------------------------------------------------------------------*
    Functions:
    同步http请求示例，分别使用了1.69和1.73的TCP连接，但其实没什么区别，新版只是封装了一下，使用方便点。
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void syncHttpRequest(int num) {
    std::cout << "syncHttpRequest: " << num << std::endl;
    try {
        //todo 如何进行https请求
        boost::asio::io_context ioc;//IO上下文

        tcp::resolver resolver{ioc};//解析器
//        tcp::socket socket{ioc};//连接实例 1.69方式
        boost::beast::tcp_stream stream{ioc};//tcp流 1.73方式

        auto const results = resolver.resolve(host, port);//解析IP

//        boost::asio::connect(socket, results.begin(), results.end());//建立连接 1.69方式
        stream.connect(results);//建立连接 1.73方式

        //设置http请求实例
        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

//        http::write(socket, req);//向连接发送请求 1.69方式
        http::write(stream, req);//发送请求 1.73方式

        boost::beast::flat_buffer buffer;//用于保留输入流

        http::response<http::dynamic_body> res;//响应体实例

//        http::read(socket, buffer, res);//将输入流读入响应体 1.69方式
        http::read(stream, buffer, res);

        std::cout << res << std::endl;//输出结果

        //关闭连接
        boost::system::error_code ec;
//        socket.shutdown(tcp::socket::shutdown_both, ec);//关闭连接 1.69方式
        stream.socket().shutdown(tcp::socket::shutdown_both, ec); // 关闭连接 1.73方式

        //若产生错误，打印信息
        if (ec && ec != boost::beast::errc::not_connected) {
            std::cout << ec.message() << std::endl;
        }

    }
    catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

/*---------------------------------------------------------------------------------------------*
    Functions:
    异步http请求示例，连接类在其他文件中。
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void asyncHttpRequest(int num) {
    std::cout << "asyncHttpRequest: " << num << std::endl;
    boost::asio::io_context ioc;
    //1.69版本异步请求
//    std::make_shared<SimpleHttpClient>(ioc)->start(host, port, target, version);
    //1.73版本请求
    std::make_shared<SimpleHttpClient2>(ioc)->run(host, port, target, version);
    //Q:怎么体现异步
    ioc.run();
}

/*---------------------------------------------------------------------------------------------*
    Functions:
    同步https请求示例
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void syncHttpsRequest() {
    try {
        boost::asio::io_context ioc;//建立IO上下文
        boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv12_client);//SSL上下文

        load_root_certificates(ctx);//

        ctx.set_verify_mode(boost::asio::ssl::verify_peer);

        tcp::resolver resolver(ioc);
        boost::beast::ssl_stream<boost::beast::tcp_stream> stream(ioc, ctx);//包装tcp流

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(stream.native_handle(), ssl_host)) {
            std::cerr << "SSL_set_tlsext_host_name Error" << std::endl;
        }

        auto const results = resolver.resolve(ssl_host, ssl_port);
        boost::beast::get_lowest_layer(stream).connect(results);
        stream.handshake(boost::asio::ssl::stream_base::client);

        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, ssl_target, version};
        req.set(http::field::host, ssl_host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        boost::beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        // Write the message to standard out
        std::cout << res << std::endl;

        // Gracefully close the stream
        boost::beast::error_code ec;
        stream.shutdown(ec);
        if (ec == boost::asio::error::eof) {
            ec = {};
        }
        if (ec)
            std::cerr << "Shutdown Error: " << ec.message() << std::endl;
    }
    catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

}