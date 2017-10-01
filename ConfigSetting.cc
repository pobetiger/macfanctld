
#include <string>
#include <fstream>
#include <algorithm>
#include <iostream>
#include "ConfigSetting.h"

static inline std::string& trim(std::string &s)
{
	if (s.size() > 0) 
	{
		s.erase(s.begin(),
			std::find_if_not(s.begin(), s.end(),
				[](int c) { return std::isspace(c); }));
		s.erase(
			std::find_if_not(s.rbegin(), s.rend(),
				[](int c) { return std::isspace(c); }).base(), s.end());
	}
	return s;
}

ConfigSetting::ConfigSetting() 
	: config{}, configReady(false)
{
}

bool ConfigSetting::load(const std::string &filename)
{
	std::ifstream ifs(filename, std::ios::in);
	if (!ifs)
		return false;

	// when we reload, we must flush/free the table elements
	config.clear();

	while(!ifs.eof()) {
		std::string line;
		getline(ifs, line);
		trim(line);

		// filter out comments and blank lines
		if ((line.size() <= 0) || (line[0]=='#'))
			continue;

		auto colon = std::find(line.begin(), line.end(), ':');

		// check if valid kv pair (must contain ':')
		//  and there must be some value after the colon
		if (colon == line.end() && (colon != (line.end()-1))) {
			std::cout << "Ill-formed line in config file: " << filename
						<< ": content=[" << line << "]\n";
			continue; // invalid line, treat as comment
		}

		// extract kv-pair
		auto key = std::string(line.begin(), colon);
		auto val = std::string(colon+1, line.end());
		// trim one more time ("mykey   " or "   myvalue" cases)
		trim(key);
		trim(val);

		config[key] = val;
	}
	ifs.close();
	configReady = true;

	return configReady;
}

void ConfigSetting::clear()
{
	config.clear();
}

