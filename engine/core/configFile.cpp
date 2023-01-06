#include "configFile.hpp"
#include "utility.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <array>

const std::string ConfigFile::DEFAULT_SETTING_SECTION_NAME = "__default_setting_section__";
ConfigFile::ConfigFile()
{
	//create default setting section
	_settingSectionMap.emplace(DEFAULT_SETTING_SECTION_NAME, SettingSection{});
}
Result ConfigFile::load(std::string_view filename,
						const char separator,
						bool trimWhitSpace)
{
	std::fstream f{std::string(filename)};

	if (!f.is_open())
	{
		THROW("failed to open file:", filename);
		//return Result::ERROR_OPEN_FILE;
	}
	std::stringstream ss;
	std::stringstream ss2;
	std::string currentSection;
	std::string keyword;
	std::stringstream value;
	SettingSection *pCurrentSettingSection = &_settingSectionMap[DEFAULT_SETTING_SECTION_NAME];
	char c;

	while (!f.eof())
	{
		//reset stringstream
		ss.str("");
		ss.clear();
		f.get(*ss.rdbuf(), '\n');
		if (f.bad())
		{
			LOG("reading file error, file name:", filename);
			return Result::ERROR_READING_FILE;
		}
		else if (f.fail())
		{
			f.clear();
			f.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // skip bad input
			continue;
		}
		//=====
		ss2.str("");
		ss2.clear();
		value.str("");
		value.clear();
		for (; ss.good(); ss.get())
		{
			c = ss.peek();
			//comment
			if (c == '#' || c == '@' || c == ';' || c == '!')
				break;
			else if (c == '[')
			{
				ss.get(); //skip [
				if (ss.get(*ss2.rdbuf(), ']'))
				{
					//new section
					currentSection = ss2.str();
					if (_settingSectionMap.find(currentSection) == _settingSectionMap.end())
						pCurrentSettingSection = &(_settingSectionMap[currentSection] = SettingSection{});
					ss.get(); //skip ]
				}
				else
				{
					LOG("ConfigFile load, not closed bracket");
					break;
				}
			}
			else if (!std::isspace(c))
			{
				if (ss.get(*ss2.rdbuf(), separator))
				{
					//find a key
					ss2 >> keyword;
					if (keyword.empty())
					{
						LOG("ConfigFile load, empty keyword, filename:", filename);
					}
					ss.get();			 //skip sparator
					ss >> value.rdbuf(); //get value
					pCurrentSettingSection->emplace(keyword, trim(value.str()));
				}
			}
		}
	}
	return Result::SUCCEED;
}
void ConfigFile::setSetting(std::string_view key,
							std::string_view value,
							std::string_view section)
{
	_settingSectionMap[std::string(section)].emplace(key, value);
}
std::string_view ConfigFile::getSetting(std::string_view key,
										  std::string_view section,
										  std::string_view defaultValue) const
{
	std::string sectionstr{section};
	std::string keystr{key};
	if (_settingSectionMap.find(sectionstr) != _settingSectionMap.end())
	{
		auto &&a = _settingSectionMap.at(sectionstr);
		if (a.find(keystr) != a.end())
			return a.at(keystr);
	}
	return defaultValue;
}
void ConfigFile::clear()
{
	_settingSectionMap.clear();
}