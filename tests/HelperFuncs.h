#ifndef HELPER_FUNCS_H
#define HELPER_FUNCS_H

#include <string>
#include <vector>

void testGet(const std::string& uri, const std::string& outPath);
void testPut(const std::string& uri, const std::string& inPath, const std::string& outPath);
void testPost(const std::string& uri, const std::vector<std::string>& parameters, const std::string& outPath);

bool compareFiles(const std::string& filePath1, const std::string& filePath2);

#endif

