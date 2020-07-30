//
// Created by 孙宇 on 2020/7/30.
//
/***********************************************************************************************
 ***                                Y S H E N - S T U D I O S                                ***
 ***********************************************************************************************
                                                                                              
                  Project Name : CppLearningDemos 
                                                                                              
                     File Name : HttpServerHandler.h 
                                                                                              
                    Programmer : 孙宇 
                                                                                              
                    Start Date : 2020/7/30 
                                                                                              
                   Last Update : 2020/7/30 
                                                                                              
 *---------------------------------------------------------------------------------------------*
  Description:           
        处理请求的各方法封装
  
 *---------------------------------------------------------------------------------------------*
  Functions:
 *---------------------------------------------------------------------------------------------*
  Updates:
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CPPLEARNINGDEMOS_REQUESTHANDLER_H
#define CPPLEARNINGDEMOS_REQUESTHANDLER_H

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
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//确定mime type，使用string_view类型更轻量，比std::string效率更高
inline beast::string_view mime_type(beast::string_view path) {
    using beast::iequals;
    auto const ext = [&path] {
        auto const pos = path.rfind(".");
        return pos == beast::string_view::npos ? beast::string_view{} : path.substr(pos);
    }();
    if (iequals(ext, ".htm")) return "text/html";
    if (iequals(ext, ".html")) return "text/html";
    if (iequals(ext, ".php")) return "text/html";
    if (iequals(ext, ".css")) return "text/css";
    if (iequals(ext, ".txt")) return "text/plain";
    if (iequals(ext, ".js")) return "application/javascript";
    if (iequals(ext, ".json")) return "application/json";
    if (iequals(ext, ".xml")) return "application/xml";
    if (iequals(ext, ".swf")) return "application/x-shockwave-flash";
    if (iequals(ext, ".flv")) return "video/x-flv";
    if (iequals(ext, ".png")) return "image/png";
    if (iequals(ext, ".jpe")) return "image/jpeg";
    if (iequals(ext, ".jpeg")) return "image/jpeg";
    if (iequals(ext, ".jpg")) return "image/jpeg";
    if (iequals(ext, ".gif")) return "image/gif";
    if (iequals(ext, ".bmp")) return "image/bmp";
    if (iequals(ext, ".ico")) return "image/vnd.microsoft.icon";
    if (iequals(ext, ".tiff")) return "image/tiff";
    if (iequals(ext, ".tif")) return "image/tiff";
    if (iequals(ext, ".svg")) return "image/svg+xml";
    if (iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}

//处理path
inline std::string path_cat(
        beast::string_view base,
        beast::string_view path
) {
    if (base.empty())
        return std::string(path);
    std::string result(base);
#ifdef BOOST_MSVC
    char constexpr path_separator = '\\';
    if (result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for (auto &c : result)
        if (c == '/')
            c = path_separator;
#else
    char constexpr path_separator = '/';
    if (result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#endif
    return result;
}

//处理请求，产生响应体，参数右值引用避免赋值提高效率
template<class Body, class Allocator, class Send>
inline void handle_request(
        beast::string_view doc_root,
        /**
         * 移动语义是C++11新增的重要功能，其重点是对右值的操作。
         * 右值可以看作程序运行中的临时结果，可以避免复制提高效率。
         */
        http::request<Body, http::basic_fields<Allocator>> &&req,
        Send &&send
) {
    //创建几种错误回复
    auto const bad_request =
            [&req](beast::string_view why) {
                http::response<http::string_body> res{http::status::bad_request, req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = std::string(why);
                res.prepare_payload();
                return res;
            };
    auto const not_found =
            [&req](beast::string_view target) {
                http::response<http::string_body> res{http::status::not_found, req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = "The resource '" + std::string(target) + "' was not found.";
                res.prepare_payload();
                return res;
            };
    auto const server_error =
            [&req](beast::string_view what) {
                http::response<http::string_body> res{http::status::internal_server_error, req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = "An error occurred: '" + std::string(what) + "'";
                res.prepare_payload();
                return res;
            };
    //不能处理的method返回bad_request
    /**
     * HTTP请求方法并不是只有GET和POST，只是最常用的。
     * 据RFC2616标准（现行的HTTP/1.1）得知，通常有以下8种方法：OPTIONS、GET、HEAD、POST、PUT、DELETE、TRACE和CONNECT。
     * 官方定义
     * HEAD方法跟GET方法相同，只不过服务器响应时不会返回消息体。一个HEAD请求的响应中，HTTP头中包含的元信息应该和一个GET请求的响应消息相同。
     * 这种方法可以用来获取请求中隐含的元信息，而不用传输实体本身。也经常用来测试超链接的有效性、可用性和最近的修改。
     * 一个HEAD请求的响应可被缓存，也就是说，响应中的信息可能用来更新之前缓存的实体。
     * 如果当前实体跟缓存实体的阈值不同（可通过Content-Length、Content-MD5、ETag或Last-Modified的变化来表明），那么这个缓存就被视为过期了。
        简而言之
        HEAD请求常常被忽略，但是能提供很多有用的信息，特别是在有限的速度和带宽下。主要有以下特点：
        1、只请求资源的首部；
        2、检查超链接的有效性；
        3、检查网页是否被修改；
        4、多用于自动搜索机器人获取网页的标志信息，获取rss种子信息，或者传递安全认证信息等
     */
    if (req.method() != http::verb::get &&
        req.method() != http::verb::head)
        return send(bad_request("Unknown HTTP-method"));
    //path验证失败的也返回bad_request
    if (req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

    //处理path
    std::string path = path_cat(doc_root, req.target());
    if (req.target().back() == '/')
        path.append("index.html");
    //处理请求
    beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);
    //404
    if (ec == beast::errc::no_such_file_or_directory)
        return send(not_found(req.target()));
    //未知错误
    if (ec)
        return send(server_error(ec.message()));

    //返回正常请求
    auto const size = body.size();
    if (req.method() == http::verb::head) {
        http::response<http::empty_body> res(http::status::ok, req.version());
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }
    /**
     * std::move相关
     * 在c++中，一个值要么是右值，要么是左值，左值是指表达式结束后依然存在的持久化对象，右值是指表达式结束时就不再存在的临时对象。所有的具名变量或者对象都是左值，而右值不具名。
     * 比如：
     * 常见的右值：“abc",123等都是右值。
     * 右值引用，用以引用一个右值，可以延长右值的生命期。
     * 为什么用右值引用
     * C++引入右值引用之后，可以通过右值引用，充分使用临时变量，或者即将不使用的变量即右值的资源，减少不必要的拷贝，提高效率
     *
     * 从实现上讲，std::move基本等同于一个类型转换：static_cast<T&&>(lvalue);
     * 此方法以非常简单的方式将左值引用转换为右值引用
     * 使用场景：1 定义的类使用了资源并定义了移动构造函数和移动赋值运算符，2 该变量即将不再使用
     */
    http::response<http::file_body> res{
            std::piecewise_construct,
            std::make_tuple(std::move(body)),
            std::make_tuple(http::status::ok, req.version())
    };
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
    /**
     * map 的特殊情况
     * map 类型的 emplace 处理比较特殊，因为和其他的容器不同，map 的 emplace 函数把它接收到的所有的参数都转发给 pair的构造函数。
     * 对于一个 pair 来说，它既需要构造它的 key 又需要构造它的 value。
     * 如果我们按照普通的 的语法使用变参模板，我们无法区分哪些参数用来构造 key, 哪些用来构造 value。 比如下面的代码：
     * map<string, complex<double>> scp;
     * scp.emplace("hello", 1, 2); // 无法区分哪个参数用来构造 key 哪些用来构造 value
     * 所以我们需要一种方式既可以接受异构变长参数，又可以区分 key 和 value，解决 方式是使用 C++11 中提供的 tuple。
     * pair<string, complex<double>> scp(make_tuple("hello"), make_tuple(1, 2));
     * 然后这种方式是有问题的，因为这里有歧义，第一个 tuple 会被当成是 key，第二 个tuple会被当成 value。
     * 最终的结果是类型不匹配而导致对象创建失败，为了解决 这个问题，C++11 设计了 piecewise_construct_t 这个类型用于解决这种歧义
     * 它是一个空类，存在的唯一目的就是解决这种歧义，全局变量 std::piecewise_construct 就是该类型的一个变量。
     */
}

//错误报告
inline void fail(beast::error_code ec, const char *what) {
    std::cerr << what << ": " << ec.message() << std::endl;
}


#endif //CPPLEARNINGDEMOS_REQUESTHANDLER_H
