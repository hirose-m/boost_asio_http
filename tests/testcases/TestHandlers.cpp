#include <boost/test/unit_test.hpp>

#include "../HelperFuncs.h"

BOOST_AUTO_TEST_SUITE(TestHandlers)

BOOST_AUTO_TEST_CASE(testGetHandler)
{
	testGet("http://localhost:8080/Hello?greeting=Hello", "./output/TestHandlers_testGetHandler.html");
	bool check = compareFiles("./data/hello.html", "./output/TestHandlers_testGetHandler.html");

	BOOST_CHECK_EQUAL(true, check);
}

BOOST_AUTO_TEST_CASE(testPostHandler)
{
	testPost("http://localhost:8080/PostForm", { "name=taro", "age=30" }, "./output/TestHandlers_testPostHandler.html");
	bool check = compareFiles("./data/post_form_response.html", "./output/TestHandlers_testPostHandler.html");
	
	BOOST_CHECK_EQUAL(true, check);
}

BOOST_AUTO_TEST_CASE(testPutHandler)
{
	testPut("http://localhost:8080/PutToNull", "./data/20k.txt", "./output/TestHandlers_testPutHandler.txt");
	bool check = compareFiles("./data/put_to_null_response.txt", "./output/TestHandlers_testPutHandler.txt");

	BOOST_CHECK_EQUAL(true, check);
}

BOOST_AUTO_TEST_SUITE_END()
