#include <boost/test/unit_test.hpp>

#include "../HelperFuncs.h"

BOOST_AUTO_TEST_SUITE(TestBasic)

BOOST_AUTO_TEST_CASE(testGetMethod)
{
	testGet("http://localhost:8080/", "./output/TestBasic_testGet.html");
	bool check = compareFiles("./data/index.html", "./output/TestBasic_testGet.html");

	BOOST_CHECK_EQUAL(true, check);
}

BOOST_AUTO_TEST_CASE(testGetMethodNotFound)
{
	testGet("http://localhost:8080/does_not_exist.html", "./output/TestBasic_testGetMethodNotFound.html");
	bool check = compareFiles("./data/404.html", "./output/TestBasic_testGetMethodNotFound.html");

	BOOST_CHECK_EQUAL(true, check);
}

BOOST_AUTO_TEST_CASE(testPutMethod)
{
	testPut("http://localhost:8080/put_20k.txt", "./data/20k.txt", "./output/TestBasic_testPutMethod.html");
	bool check = compareFiles("./data/20k.txt", "./doc/put_20k.txt");

	BOOST_CHECK_EQUAL(true, check);
}

BOOST_AUTO_TEST_SUITE_END()
