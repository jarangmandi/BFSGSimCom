#include "MetaDataUtils.h"

#include <string>
#include <sstream>
#include <regex>


MetaDataUtils::MetaDataUtils()
{
}


MetaDataUtils::~MetaDataUtils()
{
}

std::string tag = "BFSGSimCom";

std::regex MetaDataUtils::search = [] {
	std::ostringstream o;
	o << "<" << tag << ">(.*)</" << tag << ">";
	return std::regex(o.str());
}();

std::string MetaDataUtils::getMetaDataString(std::string tag, std::string data)
{
	std::string retValue = NULL;
	std::smatch m;
	
	if (std::regex_search(data, m, search))
	{
		retValue = m[1];
	}

	return retValue;
}

std::string MetaDataUtils::setMetaDataString(std::string tag, std::string data)
{
	std::string retValue;



	return retValue;
}
