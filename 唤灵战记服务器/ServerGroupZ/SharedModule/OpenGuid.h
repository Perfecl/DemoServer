#pragma once

class COpenGuid //开放功能
{
public:
	COpenGuid() = default;
	~COpenGuid() = default;

	static void Load();
	static const COpenGuid* GetOpenGuid(int type);
	int fun() const{ return fun_; }
	int open_type() const { return open_type_; }
	int parameters() const { return parameters_; }

private:
	static std::map<int, const COpenGuid*> ms_mapOpenGuid;
	int fun_{ 0 };         //功能 
	int open_type_{ 0 };    //开放类型
	int parameters_{ 0 };  //参数
};