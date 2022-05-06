#include "HelperFuncs.h"

#include <algorithm>
#include <string>
#include <boost/process.hpp>

#include <iostream>
#include <boost/filesystem.hpp>

#ifdef _WIN32
#	define CURL "C:/Windows/System32/curl.exe"
#	define DIFF "C:/Windows/System32/wsl.exe", "diff"
#else
#	define CURL "curl"
#	define DIFF "diff"
#endif

void testGet(const std::string& uri, const std::string& outPath)
{
	boost::process::system(CURL, uri, "-o", outPath);
}

void testPut(const std::string& uri, const std::string& inPath, const std::string& outPath)
{
	boost::process::system(CURL, uri, "-T", inPath, "-o", outPath);
}

void testPost(const std::string& uri, const std::vector<std::string>& parameters, const std::string& outPath)
{
	std::vector<std::string> args;
	args.push_back("-X");
	args.push_back("POST");
	for (auto p : parameters) {
		args.push_back("-d");
		args.push_back(p);
	}
	args.push_back(uri);
	args.push_back("-o");
	args.push_back(outPath);

	boost::process::child child(CURL, boost::process::args(args));
	child.join();
}

bool compareFiles(const std::string& filePath1, const std::string& filePath2)
{
	int ret = boost::process::system(DIFF, filePath1, filePath2);
	return ret == 0;
}
