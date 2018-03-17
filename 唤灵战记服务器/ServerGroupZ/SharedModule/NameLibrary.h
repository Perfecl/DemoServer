#pragma once
#include <vector>

class CNameLibrary
{
public:
	static std::string GetRandomName(bool sex);

private:
	static bool is_load;

	static std::vector<std::string> last_name;
	static std::vector<std::string> man_first_name;
	static std::vector<std::string> women_first_name;

	static void __Load();

public:
	CNameLibrary() = default;
	~CNameLibrary() = default;
};

