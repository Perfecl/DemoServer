#include "stdafx.h"
#include "Message.h"

CMessage::CMessage(const std::string& body, unsigned char protocol_type, int protocol_id)
{
	unsigned char key{ 0 };

	size_t pack_size{ body.length() + kHeaderSize + sizeof(protocol_id) };

	char pack_head[kHeaderSize]{ 0 };

	memcpy_s(pack_head, kHeaderSize, &kHeaderStart, sizeof(kHeaderStart));
	memcpy_s(pack_head + 1, kHeaderSize - 1, &pack_size, sizeof(pack_size));
	memcpy_s(pack_head + 5, kHeaderSize - 5, &protocol_type, sizeof(protocol_type));
	memcpy_s(pack_head + 6, kHeaderSize - 6, &key, sizeof(key));
	memcpy_s(pack_head + 7, kHeaderSize - 7, &kHeaderEnd, sizeof(kHeaderEnd));
	message_.append(pack_head, kHeaderSize);

	char szProtocol[4]{ 0 };
	memcpy_s(szProtocol, 4, &protocol_id, sizeof(protocol_id));
	message_.append(szProtocol, 4);
	message_.append(body);
}

CMessage::CMessage(const std::string& message)
{
	message_ = message;
}

bool CMessage::HasHeaderFormString(const char* str, size_t length)
{
	if (length < kHeaderSize)
		return false;

	if (kHeaderStart == str[0] && kHeaderEnd == str[7])
		return true;

	return false;
}

size_t	CMessage::GetSizeFromString(const char* str, size_t length)
{
	if (false == HasHeaderFormString(str, length))
		return 0;

	return *((size_t*)(str + 1));
}

CMessage CMessage::FromString(const char* str, size_t length)
{
	CMessage msg;

	size_t string_length{ GetSizeFromString(str, length) };

	if (string_length && string_length <= length)
		msg.message_.assign(str, string_length);

	return std::move(msg);
}

void CMessage::SetID(int ID)
{
	if (GetKey())
		message_.erase(12, 4);

	char id_str[4]{ 0 };
	memcpy_s(id_str, sizeof(id_str), &ID, sizeof(ID));
	message_.insert(12, id_str, sizeof(id_str));
	message_[6] = 1;

	__UpdatePackSize();
}

const char*	CMessage::body_c_str() const
{
	if (GetKey())
		return message_.c_str() + 16;
	else
		return message_.c_str() + 12;
}

size_t CMessage::body_size() const
{
	if (GetKey())
		return message_.length() - 16;
	else
		return message_.length() - 12;
}

void CMessage::ClearID()
{
	if (GetKey())
		message_.erase(12, 4);

	message_[6] = 0;

	__UpdatePackSize();
}

int	CMessage::GetID() const
{
	if (GetKey())
		return *((int*)(message_.c_str() + 12));
	return 0;
}

void CMessage::__UpdatePackSize()
{
	size_t size{ message_.length() };
	message_.erase(1, 4);
	char pack_size[sizeof(size)]{ 0 };
	memcpy_s(pack_size, sizeof(pack_size), &size, sizeof(size));
	message_.insert(1, pack_size, sizeof(pack_size));
}
