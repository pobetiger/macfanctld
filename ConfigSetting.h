
#ifndef _CONFIG_SETTING_H_
#define _CONFIG_SETTING_H_

#include <string>
#include <type_traits>
#include <unordered_map>


#include <experimental/optional>
template<typename T>
using optional = std::experimental::optional<T>;
// need gcc-7 or clang-5 to move this out of experimental

class ConfigSetting
{
private:
	std::unordered_map<std::string, std::string> config;
	bool configReady;

public:

	ConfigSetting();
	bool load(const std::string &filename);
	void clear();

	const std::unordered_map<std::string, std::string> &get()
	{
		return config;
	}

	// these allow us to get parameters as an expected type
	// SFINAE magic
	template< typename T, 
		typename = typename std::enable_if<std::is_integral<T>::value, T>::type>
	optional<T> parameter(std::string key) { 
		return config.find(key) != config.end() 
				? std::stoi(config[key])
				: optional<T> {};
	}
	template< typename T, 
		typename = typename std::enable_if<std::is_floating_point<T>::value, T>::type>
	optional<double> parameter(std::string key) { 
		return config.find(key) != config.end()
				? std::stod(config[key])
				: optional<T> {};
	}
	template<typename T = std::string, 
		typename = typename std::enable_if<!std::is_integral<T>::value &&
										   !std::is_floating_point<T>::value, T>::type>
	optional<std::string> parameter(std::string key) { 
		return config.find(key) != config.end()
				? config[key]
				: optional<T> {};
	}
};

#endif

