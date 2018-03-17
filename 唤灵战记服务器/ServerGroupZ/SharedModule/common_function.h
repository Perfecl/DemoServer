#pragma once
#include <random>

//转换到MD5码
std::string ToMD5(const std::string& str);

//获取mac地址
std::string MacAddress();

//读取文件
std::string GetDataFromFile(const char* path);

//字符转换
std::wstring ANSI_to_UNICODE(std::string src);
std::string  ANSI_to_UTF8(std::string src);
std::string  UNICODE_to_ANSI(std::wstring src);
std::string  UNICODE_to_UTF8(std::wstring src);
std::string  UTF8_to_ANSI(std::string src);
std::wstring UTF8_to_UNICODE(std::string src);

//获取名子标准长度
int	GetNameStandardLength(const std::string& name);

//拼接字符串
template<typename... Args> std::string FormatString(const char* szStr, Args... args)
{
	std::ostringstream os;
	os << szStr;
	priv::__MakeString(os, args...);
	return std::move(os.str());
}
namespace priv
{
	template<typename T, typename... Args> void __MakeString(std::ostringstream& os, T value, Args... args)
	{
		os << value;
		__MakeString(os, args...);
	}
	template<typename T> void __MakeString(std::ostringstream& os, T value)
	{
		os << value;
	}
}

//获取随机数
template<typename T> T GetRandom(T min, T max)
{
	T tMin = min < max ? min : max;
	T tMax = max > min ? max : min;

	std::uniform_int_distribution<T> axis_dist{ tMin, tMax };

	return axis_dist(std::random_device());
}


//时间函数
enum class TIME_TYPE{ SECOND, MINUTE, HOUR, DAY, WEEK, MONTH, YEAR, DATE };
enum class DURATION_TYPE{ MICROSECOND, MILLISECOND, SECOND, MINUTE, HOUR, HALF_HOUR, DAY, WEEK };

int			GetTimeValue(TIME_TYPE type, time_t timez);
__int64		GetDurationCount(time_t after, time_t before, DURATION_TYPE enType);


//是否在数组中
template<typename T> bool IsInArray(T container, int value)
{
	for (auto &it : container)
	{
		if (value == it)
			return true;
	}

	return false;
}

template<typename T> bool IsInArray(T container, int value, size_t start_index)
{
	for (size_t i = start_index; i < container.size(); i++)
	{
		if (value == container[i])
			return true;
	}

	return false;
}