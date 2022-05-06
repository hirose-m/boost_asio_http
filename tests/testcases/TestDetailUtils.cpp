#include <boost/test/unit_test.hpp>

#include "../../boost_asio_http_server.hpp"

BOOST_AUTO_TEST_SUITE(TestDetailUtils)

BOOST_AUTO_TEST_CASE(testParaseUri)
{
	std::string uri = "/method?string=xyz&int=123&encoded=foo%28%22aaa%22%29";

	std::string path;
	std::map<std::string, std::string> parameters;
	boost_asio_http::detail::utils::parase_uri(uri, path, parameters);

	BOOST_CHECK_EQUAL(std::string("/method"), path);
	BOOST_CHECK_EQUAL(3u, parameters.size());
	BOOST_CHECK_EQUAL(std::string("xyz"), parameters["string"]);
	BOOST_CHECK_EQUAL(std::string("123"), parameters["int"]);
	BOOST_CHECK_EQUAL(std::string("foo(\"aaa\")"), parameters["encoded"]);
}

BOOST_AUTO_TEST_CASE(testDecodePercentEncoding)
{
	std::string decoded1 = boost_asio_http::detail::utils::decode_percent_encoding("%21%22%23%24%25%26%27%28%29%2a%2b%2c");
	BOOST_CHECK_EQUAL("!\"#$%&'()*+,", decoded1);

	std::string decoded2 = boost_asio_http::detail::utils::decode_percent_encoding("%2d%2e%2f0123456789%3a%3b%3c%3d%3e%3f");
	BOOST_CHECK_EQUAL("-./0123456789:;<=>?", decoded2);

	std::string decoded3 = boost_asio_http::detail::utils::decode_percent_encoding("%40ABCXYZ%5B%5C%5D%5E%5F%60abcxyz%7b%7c%7d%7e");
	BOOST_CHECK_EQUAL("@ABCXYZ[\\]^_`abcxyz{|}~", decoded3);

	std::string decoded4 = boost_asio_http::detail::utils::decode_percent_encoding("%80%f0%ff");
	BOOST_CHECK_EQUAL("\x80\xf0\xff", decoded4);
}

BOOST_AUTO_TEST_SUITE_END()
