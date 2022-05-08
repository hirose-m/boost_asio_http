#include "boost_asio_http_server.hpp"

using namespace boost_asio_http;

void greeting(request& rq, response& rs)
{
    rs.set_code(response::ok);
    rs.set_content_type("text/html");
    rs.stream() << "<html><head><title>Greeting</title></head>"
                << "<body><h1>Hello, world.</h1></body></html>";
}

int main()
{
    boost_asio_http::server s("0.0.0.0", "8080", "./doc");

    s.set_get_handler("/Greeting", greeting); // resister handler against the Web API.

    s.run();  // blocking here and requests are processed asynchronously.

    return 0;
}
