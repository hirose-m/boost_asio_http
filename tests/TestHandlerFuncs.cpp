#include "TestHandlerFuncs.h"

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/null.hpp>
#include <iostream>

void hello(boost_asio_http::request& rq, boost_asio_http::response& rs)
{
	std::string greeting = rq.parameter("greeting");

	rs.set_code(boost_asio_http::response::ok);
	rs.set_content_type("text/html");

	rs.stream() << "<html><head><title>" << greeting << "</title></head><body><h1>" << greeting << "</h1></body></html>";
}

void postForm(boost_asio_http::request& rq, boost_asio_http::response& rs)
{
	std::string name = rq.parameter("name");
	std::string age = rq.parameter("age");

	rs.set_code(boost_asio_http::response::ok);
	rs.set_content_type("text/html");
	
	rs.stream() << "<html><head><title>POSTED</title></head><body><ul>"
	            << "<li>" << name << "</li>"
	            << "<li>" << age << "</li>"
	            << "</ul></body></html>";
}

void putToNull(boost_asio_http::request& rq, boost_asio_http::response& rs)
{
	std::streamsize contentLength = rq.content_length();
		
	boost::iostreams::stream<boost::iostreams::null_sink> nullOstream((boost::iostreams::null_sink()));
	nullOstream << rq.stream().rdbuf();

	rs.set_code(boost_asio_http::response::ok);
	rs.set_content_type("text/plain");

	rs.stream() << contentLength;
}