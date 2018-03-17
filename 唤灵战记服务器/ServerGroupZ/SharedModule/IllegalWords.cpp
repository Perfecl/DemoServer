#include "stdafx.h"
#include "IllegalWords.h"

std::vector<std::wstring> CIllegalWords::illegal_words_library_;

CIllegalWords::CIllegalWords()
{

}

CIllegalWords::~CIllegalWords()
{

}

void CIllegalWords::Load()
{
	dat_BLOCKWORD_Library lib_word;
	lib_word.ParseFromString(GetDataFromFile(GAME_DATA_PATH"BlockWord.txt"));

	for (int i = 0; i < lib_word.block_word_size(); i++)
	{
		std::wstring string = ANSI_to_UNICODE(lib_word.block_word(i));
		if (string.size() != 0)
			illegal_words_library_.push_back(ANSI_to_UNICODE(lib_word.block_word(i)));
	}
}

bool CIllegalWords::HasIllegalWords(const std::string& str)
{
	return HasIllegalWords(UTF8_to_UNICODE(str));
}

bool CIllegalWords::HasIllegalWords(const std::wstring& str)
{
	for (auto &it : illegal_words_library_)
	{
		if (str.find(it) != std::string::npos)
			return true;
	}

	return false;
}
