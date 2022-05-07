//
// boost_asio_http_server.hpp
//
// Copyright (c) 2022 HIROSE,Motohito
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_HTTP_HPP
#define BOOST_ASIO_HTTP_HPP

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>

#include <array>
#include <cctype>
#include <functional>
#include <ios>
#include <memory>
#include <set>
#include <string>

namespace boost_asio_http {

class request;
class response;
using handler = std::function<void(request&, response&)>;

namespace detail {

constexpr std::streamsize uninitialized_content_length = std::numeric_limits<long long>::max();

class socket_streambuf : public std::streambuf
{
public:
    socket_streambuf(boost::asio::ip::tcp::socket& socket, boost::asio::yield_context yield)
        : socket_(std::move(socket)), yield_(yield), remained_(detail::uninitialized_content_length)
    {
        setg(inBuffer_.data(), inBuffer_.data(), inBuffer_.data());
        setp(outBuffer_.data(), outBuffer_.data() + bufferSize);
    }

    void set_remained_size(std::streamsize n)
    {
        remained_ = n - std::distance(gptr(), egptr());
    }

protected:
    int underflow()
    {
        if (gptr() || gptr() >= egptr()) {
            if (remained_ <= 0) return traits_type::eof();

            boost::system::error_code ec;
            auto n = socket_.async_read_some(boost::asio::buffer(inBuffer_), yield_[ec]);
            if (ec) return traits_type::eof();
            
            if (remained_ != detail::uninitialized_content_length && remained_ >= 0) {
                remained_ -= n;
            }
            setg(inBuffer_.data(), inBuffer_.data(), inBuffer_.data() + n);
        }
        return *gptr();
    }

    int sync()
    {
        if (pbase() != pptr()) {
            boost::system::error_code ec;
            boost::asio::async_write(socket_, boost::asio::buffer(pbase(), std::distance(pbase(), pptr())), yield_[ec]);
            if (ec) return -1;

            pbump(static_cast<int>(pbase() - pptr()));
        }
        return 0;
    }

private:
    static constexpr std::streamsize bufferSize = 16 * 1024;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::yield_context yield_;
    std::array<char, bufferSize> inBuffer_, outBuffer_;
    std::streamsize remained_;
};

class utils
{
public:
    static std::string simple_response_entity(int code, const std::string& status)
    {
        std::stringstream ss;
        ss << "<html>"
           << "<head><title>" << status << "</title></head>"
           << "<body><h1>" << code << " " << status << "</h1></body>"
           << "</html>";

        return std::move(ss.str());
    }

    static void parase_uri(const std::string& uri, std::string& path, std::map<std::string, std::string>& parameters)
    {
        auto pos = uri.find_first_of('?');
        path = detail::utils::decode_percent_encoding(uri.substr(0, pos));
        if (pos != std::string::npos) {
            detail::utils::parse_parameter(uri.substr(pos + 1), parameters);
        }
    }

    static void parse_parameter(const std::string& src, std::map<std::string, std::string>& parameters)
    {
        std::vector<std::string> splitted;
        boost::algorithm::split(splitted, src, boost::is_any_of("&"));
        for (auto p : splitted) {
            auto pos = p.find_first_of('=');
            parameters[p.substr(0, pos)] = pos != std::string::npos ? detail::utils::decode_percent_encoding(p.substr(pos + 1)) : "";
        }
    }

    static std::string decode_percent_encoding(const std::string& src)
    {
        std::string result;
        char hex[3];

        for (auto it = src.begin(); it != src.end(); ++it) {
            if (*it == '%') {
                if (++it == src.end()) break;
                hex[0] = *it;
                if (++it == src.end()) break;
                hex[1] = *it;
                hex[2] = '\0';
                result.push_back(static_cast<char>(std::strtol(hex, nullptr, 16)));
            } else {
                result.push_back(*it);
            }
        }
        return std::move(result);
    }

    static std::string extension_to_mime_type(const std::string& extension)
    {
        static const std::map<std::string, std::string> table = {{"gif", "image/gif"}, {"htm", "text/html"}, {"html", "text/html"}, {"jpg", "image/jpeg"}, {"jpeg", "image/jpeg"}, {"txt", "text/plain"}, {"png", "image/png"}};

        auto it = table.find(extension.front()=='.' ? extension.substr(1) : extension);
        return it != table.end() ? it->second : "text/plain";
    }
};

class handler_table
{
public:
    handler_table() {}

    void set_get_handler(const std::string& name, handler h) { getHandlers_[name] = h; }
    void set_post_handler(const std::string& name, handler h) { postHandlers_[name] = h; }
    void set_put_handler(const std::string& name, handler h) { putHandlers_[name] = h; }

    handler get_handler(const std::string& name, handler def = empty_handler) const { return find_handler(getHandlers_, def, name); }
    handler post_handler(const std::string& name, handler def = empty_handler) const { return find_handler(postHandlers_, def, name); }
    handler put_handler(const std::string& name, handler def = empty_handler) const { return find_handler(putHandlers_, def, name); }

    static void empty_handler(request&, response&) {}
private:
    static handler find_handler(const std::map<std::string, handler>& handlers, const handler def, const std::string& name)
    {
        auto it = handlers.find(name);
        return (it != handlers.end() ? it->second : def);
    }

    std::map<std::string, handler> getHandlers_, postHandlers_, putHandlers_;
};

class connection_manager;

class connection : public std::enable_shared_from_this<connection>
{
public:
    connection(const connection&) = delete;
    connection& operator=(const connection&) = delete;

    explicit connection(boost::asio::io_context& ioContext, boost::asio::ip::tcp::socket socket, connection_manager& manager, const std::string& docRoot, handler_table& handlers)
        : strand_(ioContext.get_executor()), socket_(std::move(socket)), connectionManager_(manager), docRoot_(docRoot), handlerTable_(handlers) {}

    void start() { do_process(); }
    void stop() { socket_.close(); }

private:
    void do_process();
    void default_get_handler(request& rq, response& rs);
    void default_post_handler(request& rq, response& rs);
    void default_put_handler(request& rq, response& rs);

    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    boost::asio::ip::tcp::socket socket_;
    connection_manager& connectionManager_;
    std::string docRoot_;
    handler_table& handlerTable_;
};

typedef std::shared_ptr<connection> connection_ptr;

class connection_manager
{
public:
    connection_manager(const connection_manager&) = delete;
    connection_manager& operator=(const connection_manager&) = delete;

    connection_manager() {}

    void start(connection_ptr c)
    {
        connections_.insert(c);
        c->start();
    }

    void stop(connection_ptr c)
    {
        connections_.erase(c);
        c->stop();
    }

    void stop_all()
    {
        for (auto c : connections_) c->stop();
        connections_.clear();
    }
private:
    std::set<connection_ptr> connections_;
};

}   // namespace boost_asio_http::detail

class request
{
private:
    friend class detail::connection;

    request(detail::socket_streambuf* sb)
        : is_(sb), contentLength_(0)
    {
        std::string uri;

        for (int lineno = 0; ; lineno++) {
            std::string line;
            std::getline(is_, line, '\n');
            if (is_.eof()) break;

            if(line.back()=='\r') line.pop_back();
            
            if (lineno++ == 0) {
                std::stringstream ss(line);
                ss >> method_ >> uri >> protocol_;
            } else {
                const std::string contentLengthTag = "Content-Length:";
                const std::string contentTypeTag = "Content-Type:";
                if (line.find(contentLengthTag) == 0) {
                    contentLength_ = std::strtol(line.data() + contentLengthTag.length(), nullptr, 10);
                } else if (line.find(contentTypeTag) == 0) {
                    contentType_ = line.substr(contentTypeTag.length());
                    if (std::isspace(contentType_.front())) contentType_.erase(0, 1);
                }
            }

            if (line.empty()) break;
        }

        detail::utils::parase_uri(uri, path_, parameters_);

        if (method_ == "POST" || method_ == "PUT") sb->set_remained_size(contentLength_);

        if (method_ == "POST" && contentType_ == "application/x-www-form-urlencoded") {
            std::stringstream data;
            data << is_.rdbuf();
            detail::utils::parse_parameter(data.str(), parameters_);
        }
    }

public:
    std::string method() const { return method_; }
    std::string path() const { return path_; }
    std::string protocol() const { return protocol_; }

    std::streamsize content_length() const { return contentLength_; }
    std::string content_type() const { return contentType_; }
    std::istream& stream() { return is_; }

    std::vector<std::string> parameter_names() const
    {
        std::vector<std::string> result;
        for (auto e : parameters_) {
            result.push_back(e.first);
        }
        return result;
    }

    std::string parameter(const std::string& name) const
    {
        auto it = parameters_.find(name);
        return it != parameters_.end() ? it->second : "";
    }
private:
    std::istream is_;
    std::string method_, path_, protocol_;
    std::map<std::string, std::string> parameters_;
    std::streamsize contentLength_;
    std::string contentType_;
};

class response
{
public:
    enum code { ok = 200, bad_request = 400, forbidden = 403, not_found = 404, internal_server_error = 500 };
private:
    friend class detail::connection;

    response(detail::socket_streambuf* sb)
        : os_(sb), code_(ok), headerWritten_(false), contentType_("text/html"), contentLength_(detail::uninitialized_content_length)
    {
    }
    void flush_header()
    {
        if (headerWritten_) return;

        os_ << "HTTP/1.1 " << code_ << " " << status(code_) << "\r\n";
        os_ << "Content-Type: " << contentType_ << "\r\n";
        if (contentLength_ != detail::uninitialized_content_length) {
            os_ << "Content-Length: " << contentLength_ << "\r\n";
        }
        os_ << "\r\n";

        headerWritten_ = true;
    }

    static std::string status(code c)
    {
        static std::map<code, std::string> table = { {ok, "OK"}, {bad_request, "Bad Request"}, {forbidden, "Forbidden"}, {not_found, "Not Found"}, {internal_server_error, "Internal Server Error"}};
        auto it = table.find(c);
        return it != table.end() ? it->second : "OK";
    }

public:
    void close()
    {
        if (closed_) return;

        flush_header();
        os_.flush();
    }

    void set_code(code code) { code_ = code; }
    void set_content_type(const std::string& type) { contentType_ = type; }
    void set_content_length(std::streamsize n) { contentLength_ = n; }

    std::ostream& stream()
    {
        flush_header();
        return os_;
    }

    void simple_response(code code)
    {
        std::string entity = detail::utils::simple_response_entity(code, status(code));

        set_code(code);
        set_content_length(entity.length());
        set_content_type("text/html");

        std::ostream& os = stream();
        os << entity;
        os.flush();
    }
private:
    std::ostream os_;
    bool headerWritten_;
    bool closed_;
    code code_;
    std::string contentType_;
    std::streamsize contentLength_;
};

inline void detail::connection::do_process()
{
    auto self(shared_from_this());

    boost::asio::spawn(strand_, [this, self](boost::asio::yield_context yield) {
        try {
            boost::system::error_code ec;
            bool headerReceived = false;

            socket_streambuf sb(socket_, yield);
            request rq(&sb);
            response rs(&sb);

            handler h = detail::handler_table::empty_handler;
            if (rq.method() == "GET") {
                h = handlerTable_.get_handler(rq.path(), [&](request& rq, response& rs) { return default_get_handler(rq, rs); });
            } else if (rq.method() == "POST") {
                h = handlerTable_.post_handler(rq.path(), [&](request& rq, response& rs) { return default_post_handler(rq, rs); });
            } else if (rq.method() == "PUT") {
                h = handlerTable_.put_handler(rq.path(), [&](request& rq, response& rs) { return default_put_handler(rq, rs); });
            }
            h(rq, rs);

            rs.close();
        } catch (...) {
        }

        connectionManager_.stop(shared_from_this());
    });
}

inline void detail::connection::default_get_handler(request& rq, response& rs)
{
    if (rq.path().empty() || rq.path().front()!='/' || rq.path().find("..") != std::string::npos) { // prevent path traversal attack
        rs.simple_response(response::bad_request);
        return;
    }

    boost::filesystem::path path(docRoot_);
    path /= (rq.path()=="/" ? std::string("index.html") : rq.path());

    if (!boost::filesystem::exists(path)) {
        rs.simple_response(response::not_found);
        return;
    }

    auto fileSize = boost::filesystem::file_size(path);
    std::fstream file(path.string(), std::ios::in|std::ios::binary);
    if (!file.is_open()) {
        rs.simple_response(response::not_found);
        return;
    }

    rs.set_code(response::ok);
    rs.set_content_length(fileSize);
    rs.set_content_type(detail::utils::extension_to_mime_type(path.extension().string()));

    std::ostream& os = rs.stream();
    os << file.rdbuf();
    os.flush();
}

inline void detail::connection::default_post_handler(request& rq, response& rs)
{
    rs.simple_response(response::bad_request);
}

inline void detail::connection::default_put_handler(request& rq, response& rs)
{
    boost::filesystem::path path(docRoot_);
    path /= rq.path();

    std::fstream file(path.string(), std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        rs.simple_response(response::forbidden);
        return;
    }

    std::istream& is = rq.stream();
    file << is.rdbuf();
    file.close();

    rs.set_code(response::ok);
    rs.set_content_length(0);
    rs.set_content_type("text/html");
}

class server
{
public:
    server(const server&) = delete;
    server& operator=(const server&) = delete;

    explicit server(const std::string& address, const std::string& port, const std::string& docRoot)
        : ioContext_(1), acceptor_(ioContext_), docRoot_(docRoot), acceptorOpened_(false)
    {
        boost::asio::ip::tcp::resolver resolver(ioContext_);
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(address, port).begin();
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();
        acceptorOpened_ = true;

        do_accept();
    }

    void run() { ioContext_.run(); }

    void stop()
    {
        acceptorOpened_ = false;
        acceptor_.close();
        connectionManager_.stop_all();
    }

    // API registration
    void set_get_handler(const std::string& name, handler h) { handlerTable_.set_get_handler(name, h); }
    void set_post_handler(const std::string& name, handler h) { handlerTable_.set_post_handler(name, h); }
    void set_put_handler(const std::string& name, handler h) { handlerTable_.set_put_handler(name, h); }

private:
    void do_accept()
    {
        acceptor_.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!acceptorOpened_ || !acceptor_.is_open()) return;

            if (!ec) {
                connectionManager_.start(std::make_shared<detail::connection>(ioContext_, std::move(socket), connectionManager_, docRoot_, handlerTable_));
            }
            do_accept();
        });
    }

    boost::asio::io_context ioContext_;
    boost::asio::ip::tcp::acceptor acceptor_;
    bool acceptorOpened_;

    detail::connection_manager connectionManager_;
    std::string docRoot_;
    detail::handler_table handlerTable_;
};

}   // namespace boost_asio_http

#endif // BOOST_ASIO_HTTP_HPP
