#ifndef TEST_HANDLERS_H
#define TEST_HANDLERS_H

#include "../boost_asio_http_server.hpp"

void hello(boost_asio_http::request& rq, boost_asio_http::response& rs);
void postForm(boost_asio_http::request& rq, boost_asio_http::response& rs);
void putToNull(boost_asio_http::request& rq, boost_asio_http::response& rs);

#endif

