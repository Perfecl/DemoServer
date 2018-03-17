#pragma once
#include "..\SharedModule\NetworkZ\NetworkZ.h"

class CMessage
{
public:
	static const size_t kHeaderSize{ 8 };											//��ͷ����
	static const char   kHeaderStart{ 125 };										//��ͷ
	static const char   kHeaderEnd{ 127 };											//��ͷβ

	static bool			HasHeaderFormString(const char* str, size_t length);		//�ַ����Ƿ������ͷ
	static size_t		GetSizeFromString(const char* str, size_t length);			//���ַ����л�ȡ����
	static CMessage		FromString(const char* str, size_t length);					//���ַ����л�ȡ��Ϣ

public:
	CMessage() = default;
	CMessage(const std::string& message);
	CMessage(const std::string& body, unsigned char protocol_type, int protocol_id);
	~CMessage() = default;

	operator const char*(){ return message_.c_str(); }

	inline size_t					length() const{ return message_.length(); }									//��ȡ����

	inline networkz::ISession*		session() const { return session_; }										//��ȡ�Ự
	inline void						session(networkz::ISession* pSession){ session_ = pSession; }				//���ûỰ

	const char*						body_c_str() const;															//����ָ��
	size_t							body_size() const;															//���峤��

	inline const std::string&		message() const { return message_; }										//��ȡ��

	inline int						GetProtocolID() const { return *((int*)(message_.c_str() + 8)); }			//��ȡЭ���
	inline unsigned char			GetType() const { return *((unsigned char*)(message_.c_str() + 5)); }		//��ȡ����
	inline unsigned char			GetKey() const { return *((unsigned char*)(message_.c_str() + 6)); }		//��ȡKey
	int								GetID() const;																//��ȡID

	inline bool						IsEmpty() const { return message_.empty(); }								//�Ƿ��ǿյ�

	void							SetID(int ID);																//����ID
	void							ClearID();																	//���ID

private:
	std::string				message_;																			//��Ϣ����
	networkz::ISession*		session_{ nullptr };																//��Ϣ����Ӧ�ĻỰ

	void	__UpdatePackSize();																					//���°���
};
