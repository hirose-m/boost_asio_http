boost_asio_http
===============

## Overview

- *boost_asio_http* is a cross-platform asynchronous HTTP server library with single header file using Boost.Asio.
- If you are already using Boost in your development, you can embed HTTP server easily in your application by just including "boost_asio_http_server.hpp".

## Description

- The *boost_asio_http* library provides embedded HTTP server in your application.
- The features of this library are:
 - Simply include one header file for easy integration into your application.
 - Easily host Web API from your application.
 - Can expect the memory consumption of the HTTP server function to be low.

- There are several ways to embed HTTP server in your application, but the *boost_asio_http* provides the easiest way; just including the one header file.

- One of the motivation for embedding HTTP server in an application is to exchange data for interprocess communication.
To exchange data efficiently, it is necessary to send and receive data of the application directly without using files.
This *boost_asio_http* makes it easy to implement Web APIs which handle application data to exchange data over HTTP.

- HTTP servers implemented using Boost.Asio until now tend to consume memory and limit data size when reciving and responding requests.
This *boost_asio_http* has no data-size limitation during processing request / response and can reduce memory consumption.

## Usage

- To use this library, copy "boost_asio_http_server.hpp" to your project folder and include it from your source code.
- Test suits using Boost.Test are in tests folder.
- Example codes are in examples folder (the folder is empty just now, examples will be added soon).

## Requirement

- Boost (https://www.boost.org/)
 - development and test are done with boost 1.77.0 and VisualStudio2022, but other versions of boost and other development environments should work as well.

## Examples

- Working examples are in examples folder and tests folder.

#### Hosting files
````
#include "boost_asio_http_server.hpp"

using namespace boost_asio_http;

int main()
{
    server s("localhost", "8080", "./doc");
        // By default, GET request can get files from the document root,
        // and PUT request can put files to the document root.
    s.run();
        // run() blocks here and requests from clients are processed asynchronously.

    return 0;
}

````

#### Providing Web API
````
#include "boost_asio_http_server.hpp"

using namespace boost_asio_http;

void greeting(request& rq, response& rs)
{
    rs.set_status(response::ok);
    rs.set_conent_type("text/html");
    rs.stream() << "<html><head><title>Greeting</title></head>"
                << "<body><h1>Hello, world.</h1></body></html>";
}

int main()
{
    boost_asio_http::server s("localhost", "8888", "./doc");

    s.set_get_handler("/Greeging", greeting); // resister handler against the Web API.

    s.run();  // blocking here and requests are processed asynchronously.

    return 0;
}
````
- tests/TestHandlerFuncs.cpp are also showing examples how to implement Web API handler.


#### Run the HTTP server in thread.
- See tests/main.cpp

````
class SetupTestServer
{
public:
	SetupTestServer()
	{
		std::cout << "starting server..." << std::endl;
		server_ = std::make_shared<boost_asio_http::server>("localhost", "8080", "./doc");

		thread_ = std::make_shared<std::thread>(&boost_asio_http::server::run, server_.get());
	}

	~SetupTestServer()
	{
		server_->stop();
		thread_->join();
		std::cout << "server terminated." << std::endl;
	}
private:
	std::shared_ptr<boost_asio_http::server> server_;
	std::shared_ptr<std::thread> thread_;
};
````

## Future Work

- Following supports will be required:
 - TLS
 - authentication
etc.

## Author

- HIROSE,Motohito
 - Developer regarding simulation software.

## License

- *boost_asio_http* is under [Boost Software License](https://www.boost.org/LICENSE_1_0.txt)
