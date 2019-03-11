#pragma once

#include <string>
#include <regex>

class MetaDataUtils
{
public:
	MetaDataUtils();
	~MetaDataUtils();

	static std::string getMetaDataString(std::string, std::string);
	static std::string setMetaDataString(std::string, std::string);

	static std::string tag;
	static std::regex search;

};

