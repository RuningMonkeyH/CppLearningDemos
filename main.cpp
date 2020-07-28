#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <string>
#include "src/SimpleHttpClient.h"
#include "src/SimpleHttpClient2.h"
#include "src/ROOT_CERITICATES.h"
#include "src/SimpleHttpsClient.h"
#include <boost/thread/xtime.hpp>
//测试clion上传github
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

auto const host = "www.boost.org";
//auto const ssl_host = "cba.upk.net";
auto const ssl_host = "www.digicert.com";
auto const port = "80";
auto const ssl_port = "443";
auto const target = "/doc/libs/1_73_0/libs/beast/doc/html/index.html";
auto const ssl_target = "/";
int version = 11;

void syncHttpRequest(int num);

void asyncHttpRequest(int num);

void syncHttpsRequest();

void asyncHttpsRequest();

void coHttpRequest();

void coHttpsRequest();

int main() {
    //计时
    auto start_time = boost::get_system_time();
    std::cout << "Ready, GO!" << std::endl << std::endl;

    for (int i = 0; i < 1; ++i) {
//        syncHttpRequest(i);
//        asyncHttpRequest(i);
//        syncHttpsRequest();
//        asyncHttpsRequest();
//        coHttpRequest();
//        coHttpsRequest();
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
//        boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23_client);//SSL上下文

        load_root_certificates(ctx);//加载证书
//        ctx.load_verify_file("../3583482__upk.net_apache/3583482__upk.net_chain.crt");//本地加载证书,使用代码所在路径要用 ../
//        ctx.use_private_key_file("../3583482__upk.net_apache/3583482__upk.net.key",boost::asio::ssl::context::file_format::pem);
//        ctx.use_certificate_chain_file("../3583482__upk.net_apache/3583482__upk.net_chain.crt");
//        ctx.set_default_verify_paths(); //todo 所有方法里只有这个能用，为啥. 等写完异步和协程的再来看看
//        ctx.add_verify_path("../3583482__upk.net_nginx/");

        ctx.set_verify_mode(boost::asio::ssl::verify_peer);
//        ctx.set_verify_callback(boost::asio::ssl::rfc2818_verification(ssl_host));

        tcp::resolver resolver(ioc);
        boost::beast::ssl_stream<boost::beast::tcp_stream> stream(ioc, ctx);//包装tcp流

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(stream.native_handle(), ssl_host)) {
            boost::beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
            std::cerr << ec.message() << "\n";
            return;
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

/*---------------------------------------------------------------------------------------------*
    Functions:
    异步https请求示例，与同步一样存在证书验证问题
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void asyncHttpsRequest() {
    boost::asio::io_context ioc;
    ssl::context ctx(ssl::context::tlsv12_client);
    load_root_certificates(ctx);
//    ctx.set_default_verify_paths();
    ctx.set_verify_mode(ssl::verify_peer);
    std::make_shared<SimpleHttpsClient>(
            boost::asio::make_strand(ioc),
            ctx)->run(ssl_host, ssl_port, ssl_target, version);
    ioc.run();
}

void fail(boost::beast::error_code ec, char const *what) {
    std::cerr << what << ": " << ec.message() << std::endl;
}

/*---------------------------------------------------------------------------------------------*
    Functions:
    协程版http示例，体会与异步的不同
    当协程运行到异步阶段，将会挂起，等异步结果产生时继续协程
    目前感觉就是代码逻辑上更清晰些，不需要编写大量的回调函数，像同步一样写步骤
    应该比线程轻量一些，但是目前体会不到
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void do_session(
        std::string const &host_,
        std::string const &port_,
        std::string const &target_,
        int version_,
        boost::asio::io_context &ioc,
        const boost::asio::yield_context &yield
) {
    boost::beast::error_code ec;

    tcp::resolver resolver(ioc);
    boost::beast::tcp_stream stream(ioc);

    auto const results = resolver.async_resolve(host_, port_, yield[ec]);
    if (ec)
        return fail(ec, "resolve");
    stream.expires_after(std::chrono::seconds(30));
    //进行连接，回调改为用yield方式
    stream.async_connect(results, yield[ec]);
    if (ec)
        return fail(ec, "connect");
    //设置请求信息
    http::request<http::empty_body> req{http::verb::get, target_, version_};
    req.set(http::field::host, host_);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    stream.expires_after(std::chrono::seconds(30));
    //发送请求
    http::async_write(stream, req, yield[ec]);
    if (ec)
        return fail(ec, "write");
    //接收请求
    boost::beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;
    http::async_read(stream, buffer, res, yield[ec]);
    if (ec)
        return fail(ec, "read");
    std::cout << res << std::endl;
    //关闭连接
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    if (ec && ec != boost::beast::errc::not_connected)
        return fail(ec, "shutdown");
}

void coHttpRequest() {
    boost::asio::io_context ioc;
    //spawn绑定协程请求
    boost::asio::spawn(ioc, std::bind(
            &do_session,
            std::string(host),
            std::string(port),
            std::string(target),
            version,
            std::ref(ioc),
            std::placeholders::_1
    ));

    ioc.run();
}

//参数用std::string的目的应该和sdk备用通道模块一样，防止回调时const char*类型被回收变成野指针
void do_ssl_session(
        std::string const &host_,
        std::string const &port_,
        std::string const &target_,
        int version_,
        boost::asio::io_context &ioc,
        ssl::context &ctx,
        boost::asio::yield_context yield
) {
    boost::beast::error_code ec;
    //解析器及tcp流包装初始化
    tcp::resolver resolver(ioc);
    boost::beast::ssl_stream<boost::beast::tcp_stream> stream(ioc, ctx);
    /**
     * SNI（Server Name Indication）定义在RFC 4366，是一项用于改善SSL/TLS的技术，在SSLv3/TLSv1中被启用。
     * 它允许客户端在发起SSL握手请求时（具体说来，是客户端发出SSL请求中的ClientHello阶段），就提交请求的Host信息，使得服务器能够切换到正确的域并返回相应的证书。
     * 在 TLSv1.2（OpenSSL 0.9.8）版本开始支持。
    */
    if (!SSL_set_tlsext_host_name(stream.native_handle(), host_.c_str())) {
        ec.assign(static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category());
        std::cerr << ec.message() << std::endl;
        return;
    }
    //解析域名
    auto const results = resolver.async_resolve(host_, port_, yield[ec]);
    if (ec)
        return fail(ec, "resolve");
    //设置超时
    boost::beast::get_lowest_layer(stream)
            .expires_after(std::chrono::seconds(30));
    //连接
    boost::beast::get_lowest_layer(stream)
            .async_connect(results, yield[ec]);
    if (ec)
        return fail(ec, "connect");
    //设置超时并握手
    boost::beast::get_lowest_layer(stream)
            .expires_after(std::chrono::seconds(30));
    stream.async_handshake(ssl::stream_base::client, yield[ec]);
    if (ec)
        return fail(ec, "handshake");
    //设置http请求体并发出请求
    http::request<http::empty_body> req{http::verb::get, target, version};
    req.set(http::field::host, host_);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    boost::beast::get_lowest_layer(stream)
            .expires_after(std::chrono::seconds(30));
    http::async_write(stream, req, yield[ec]);
    if (ec)
        return fail(ec, "write");
    //创建流接收响应体
    http::response<http::dynamic_body> res;
    boost::beast::flat_buffer buffer;
    http::async_read(stream, buffer, res, yield[ec]);
    if (ec)
        return fail(ec, "read");
    std::cout << res << std::endl;
    //关闭流
    boost::beast::get_lowest_layer(stream)
            .expires_after(std::chrono::seconds(30));
    stream.async_shutdown(yield[ec]);
    if (ec == boost::asio::error::eof) {
        ec = {};
    }
    if (ec)
        return fail(ec, "shutdown");

}

void coHttpsRequest() {
    boost::asio::io_context ioc;
    ssl::context ctx(ssl::context::tlsv12_client);
//    load_root_certificates(ctx);
    ctx.set_default_verify_paths();
    ctx.set_verify_mode(ssl::verify_peer);
    boost::asio::spawn(ioc, std::bind(
            &do_ssl_session,
            std::string(ssl_host),
            std::string(ssl_port),
            std::string(ssl_target),
            version,
            /**
             std::ref 用于包装按引用传递的值。
             std::cref 用于包装按const引用传递的值。
             为什么需要std::ref和std::cref
             bind()是一个函数模板，它的原理是根据已有的模板，生成一个函数，但是由于bind()不知道生成的函数执行的时候，传递进来的参数是否还有效。
             所以它选择参数值传递而不是引用传递。如果想引用传递，std::ref和std::cref就派上用场了。
            */
            std::ref(ioc),
            std::ref(ctx),
            std::placeholders::_1
    ));
    ioc.run();
}