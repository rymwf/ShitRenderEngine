#include "utility.hpp"

std::string_view trimL(std::string_view str)
{
	while (!str.empty() && std::isspace(str.front()))
		str.remove_prefix(1);
	return str;
}
std::string_view trimR(std::string_view str)
{
	while (!str.empty() && std::isspace(str.back()))
		str.remove_suffix(1);
	return str;
}
std::string_view trim(std::string_view str)
{
	str = trimL(str);
	str = trimR(str);
	return str;
}

std::vector<std::string_view> split(std::string_view str, std::string_view delim)
{
	std::vector<std::string_view> ret;
	size_t i = 0, p = 0, s = delim.size();
	while ((p = str.find(delim, i)) != std::string_view::npos)
	{
		ret.emplace_back(str.begin() + i, str.begin() + p);
		i = p + s;
	}
	if (i < str.size())
		ret.emplace_back(str.begin() + i, str.end());
	return ret;
}
std::string_view getRelativePath(std::string_view rootPath, std::string_view fullpath)
{
	auto s = trim(rootPath);
	auto p = (std::min)(fullpath.find(s.data()), fullpath.size());
	return p == fullpath.size() ? std::string_view{} : std::string_view{fullpath.begin() + p + s.size(), fullpath.end()};
}

void parseResourceName(std::string_view resourceName, std::string_view& archiveName, std::string_view& childItemName)
{
	std::string_view a{"\\/"};
	auto p = resourceName.find_first_of(a);
	if (p == std::string_view::npos)
	{
		childItemName = resourceName;
		archiveName = {};
	}
	else
	{
		archiveName = std::string_view{resourceName.begin(), resourceName.begin() + p};
		childItemName = std::string_view{resourceName.begin() + p + 1, resourceName.end()};
	}
}