#pragma once
#include <boost\lexical_cast.hpp>

template<typename T>
class CSerializer
{
public:
	CSerializer() = default;
	~CSerializer() = default;

	inline void		AddValue(T value){ value_array_.push_back(value); }
	inline void		AddPair(int key, T value){ value_map_.push_back(std::make_pair(key, value)); }
	inline size_t	ValueSize(){ retrun value_array_.size(); }

	static std::vector<T> ParseFromString(const std::string& str)
	{
		std::vector<T>	temp_vct;
		std::string		temp_str(str);
		int				index{ -1 };

		try
		{
			while (-1 != (index = temp_str.find_first_of('|')))
			{
				temp_vct.push_back(boost::lexical_cast<T>(temp_str.substr(0, index)));
				temp_str.erase(0, index + 1);
			}
		}
		catch (std::exception exc)
		{
			RECORD_ERROR("序列化格式错误");
			return{};
		}

		return std::move(temp_vct);
	}

	static std::vector<std::pair<int, T>> ParsePairFromString(const std::string& str)
	{
		std::vector<std::pair<int, T>>	temp_map;
		std::string						temp_str(str);
		int								index{ -1 };

		try
		{
			while (-1 != (index = temp_str.find_first_of('|')))
			{
				std::string str_pair = temp_str.substr(0, index);
				int index_p = str_pair.find_first_of(",");
				int key = boost::lexical_cast<int>(str_pair.substr(0, index_p));
				str_pair.erase(0, index_p + 1);
				T value = boost::lexical_cast<T>(str_pair);
				temp_map.push_back(std::make_pair(key, value));
				temp_str.erase(0, index + 1);
			}
		}
		catch (std::exception exc)
		{
			RECORD_ERROR("序列化格式错误");
			return{};
		}

		return std::move(temp_map);
	}

	std::string SerializerToString()
	{
		os_buffer_.str("");
		for (auto &it : value_array_)
			os_buffer_ << it << "|";
		return std::move(os_buffer_.str());
	}

	std::string SerializerPairToString()
	{
		os_buffer_.str("");
		for (auto &it : value_map_)
			os_buffer_ << it.first << "," << it.second << "|";
		return std::move(os_buffer_.str());
	}

private:
	std::vector<T>					value_array_;
	std::vector<std::pair<int, T>>	value_map_;
	std::ostringstream				os_buffer_;
};
