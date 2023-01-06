#pragma once
#include "predefines.hpp"
#include "enums.hpp"
#include <unordered_map>
#include <string>
#include <any>

/**
 * @brief TODO: write function
 * 
 */
class ConfigFile
{
public:
	using SettingSection = std::unordered_map<std::string, std::string>;
	using SettingSectionMap = std::unordered_map<std::string, SettingSection>;

	static const std::string DEFAULT_SETTING_SECTION_NAME;

	ConfigFile();

	Result load(std::string_view filename,
				const char separator = '=',
				bool trimWhitSpace = true);
	constexpr SettingSectionMap &getSettingSectionMap() { return _settingSectionMap; }
	std::string_view getSetting(std::string_view key,
								  std::string_view section = DEFAULT_SETTING_SECTION_NAME,
								  std::string_view defaultValue = "") const;
	void setSetting(std::string_view key,
					std::string_view value,
					std::string_view section = DEFAULT_SETTING_SECTION_NAME);

	void clear();

private:
	SettingSectionMap _settingSectionMap;
	std::string _filename;
};