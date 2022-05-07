#define BOOST_TEST_MAIN

#include "../boost_asio_http_server.hpp"
#include "TestHandlerFuncs.h"

#include <boost/test/included/unit_test.hpp>

#include <boost/filesystem.hpp>

#include <thread>
#include <memory>
#include <iostream>

class SetupTestServer
{
public:
	SetupTestServer()
	{
		std::cout << "starting server..." << std::endl;
		server_ = std::make_shared<boost_asio_http::server>("0.0.0.0", "8080", "./doc");

		server_->set_get_handler("/Hello", hello);
		server_->set_post_handler("/PostForm", postForm);
		server_->set_put_handler("/PutToNull", putToNull);

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

class SetupTestEnvironment
{
public:
	SetupTestEnvironment()
	{
		boost::system::error_code ec;
		boost::filesystem::remove_all("./output", ec);
		boost::filesystem::create_directories("./output", ec);
	}

	~SetupTestEnvironment()
	{
	}
};

BOOST_TEST_GLOBAL_FIXTURE(SetupTestServer);
BOOST_TEST_GLOBAL_FIXTURE(SetupTestEnvironment);
