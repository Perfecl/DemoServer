#pragma once
#include <regex>

class CIllegalWords
{
public:
	CIllegalWords();
	~CIllegalWords();

	static void  Load();

	static bool HasIllegalWords(const std::string& str);
	static bool HasIllegalWords(const std::wstring& str);

	static bool IsIllegalWords(std::string str);
private:
	static std::vector<std::wstring>	illegal_words_library_;		//√Ù∏–¥ ø‚
};

