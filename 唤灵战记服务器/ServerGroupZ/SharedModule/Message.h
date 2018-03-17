#pragma once
#include "..\SharedModule\NetworkZ\NetworkZ.h"

class CMessage
{
public:
	static const size_t kHeaderSize{ 8 };											//包头长度
	static const char   kHeaderStart{ 125 };										//包头
	static const char   kHeaderEnd{ 127 };											//包头尾

	static bool			HasHeaderFormString(const char* str, size_t length);		//字符串是否包含包头
	static size_t		GetSizeFromString(const char* str, size_t length);			//从字符串中获取包长
	static CMessage		FromString(const char* str, size_t length);					//从字符串中获取消息

public:
	CMessage() = default;
	CMessage(const std::string& message);
	CMessage(const std::string& body, unsigned char protocol_type, int protocol_id);
	~CMessage() = default;

	operator const char*(){ return message_.c_str(); }

	inline size_t					length() const{ return message_.length(); }									//获取长度

	inline networkz::ISession*		session() const { return session_; }										//获取会话
	inline void						session(networkz::ISession* pSession){ session_ = pSession; }				//设置会话

	const char*						body_c_str() const;															//包体指针
	size_t							body_size() const;															//包体长度

	inline const std::string&		message() const { return message_; }										//获取包

	inline int						GetProtocolID() const { return *((int*)(message_.c_str() + 8)); }			//获取协议号
	inline unsigned char			GetType() const { return *((unsigned char*)(message_.c_str() + 5)); }		//获取类型
	inline unsigned char			GetKey() const { return *((unsigned char*)(message_.c_str() + 6)); }		//获取Key
	int								GetID() const;																//获取ID

	inline bool						IsEmpty() const { return message_.empty(); }								//是否是空的

	void							SetID(int ID);																//设置ID
	void							ClearID();																	//清空ID

private:
	std::string				message_;																			//消息缓存
	networkz::ISession*		session_{ nullptr };																//消息所对应的会话

	void	__UpdatePackSize();																					//更新包长
};
