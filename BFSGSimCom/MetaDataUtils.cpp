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

const std::string MetaDataUtils::tag = "BFSGSimCom";

std::regex MetaDataUtils::search = [] {
	std::ostringstream o;
	o << "<" << tag << ">(.*)</" << tag << ">";
	return std::regex(o.str());
}();

std::string MetaDataUtils::getMetaDataString(std::string data)
{
	std::string retValue = "";
	std::smatch m;
	
	if (std::regex_search(data, m, search))
	{
		retValue = m[1];
	}

	return retValue;
}

std::string MetaDataUtils::setMetaDataString(std::string insert, std::string data)
{
	std::string retValue;
	std::string subst;
	std::ostringstream o;

	o << "<" << tag << ">" << insert << "</" << tag << ">";
	subst = o.str();

	retValue = std::regex_replace(data, search, subst);

	// If the "modified" string hasn't changed
	if (retValue == data)
	{
		// If the original string doesn't contain the search string...
		if (getMetaDataString(data) == "")
		{
			// ... append the substitution string
			const std::string &temp = o.str();
			o.seekp(0);
			o << data;
			if (data.length() != 0) o << "\n";
			o << temp;
			retValue = o.str();
		}
	}

	return retValue;
}
