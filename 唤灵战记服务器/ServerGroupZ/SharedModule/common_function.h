#pragma once
#include <random>

//ת����MD5��
std::string ToMD5(const std::string& str);

//��ȡmac��ַ
std::string MacAddress();

//��ȡ�ļ�
std::string GetDataFromFile(const char* path);

//�ַ�ת��
std::wstring ANSI_to_UNICODE(std::string src);
std::string  ANSI_to_UTF8(std::string src);
std::string  UNICODE_to_ANSI(std::wstring src);
std::string  UNICODE_to_UTF8(std::wstring src);
std::string  UTF8_to_ANSI(std::string src);
std::wstring UTF8_to_UNICODE(std::string src);

//��ȡ���ӱ�׼����
int	GetNameStandardLength(const std::string& name);

//ƴ���ַ���
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

//��ȡ�����
template<typename T> T GetRandom(T min, T max)
{
	T tMin = min < max ? min : max;
	T tMax = max > min ? max : min;

	std::uniform_int_distribution<T> axis_dist{ tMin, tMax };

	return axis_dist(std::random_device());
}


//ʱ�亯��
enum class TIME_TYPE{ SECOND, MINUTE, HOUR, DAY, WEEK, MONTH, YEAR, DATE };
enum class DURATION_TYPE{ MICROSECOND, MILLISECOND, SECOND, MINUTE, HOUR, HALF_HOUR, DAY, WEEK };

int			GetTimeValue(TIME_TYPE type, time_t timez);
__int64		GetDurationCount(time_t after, time_t before, DURATION_TYPE enType);


//�Ƿ���������
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