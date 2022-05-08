#include "boost_asio_http_server.hpp"

using namespace boost_asio_http;

int main()
{
    server s("0.0.0.0", "8080", "./doc");
        // By default, GET request can get files from the document root,
        // and PUT request can put files to the document root.
    s.run();
        // run() blocks here and requests from clients are processed asynchronously.

    return 0;
}
