#pragma once

#ifdef NETWORKZ_EXPORTS
#define NETWORKZ_API __declspec(dllexport)
#else
#define NETWORKZ_API __declspec(dllimport)
#endif

namespace networkz
{
	__interface ISession
	{
		void					send(const char* str, size_t len);											//����
		size_t					data_length() const; 														//��ȡ���ݳ���
		const char*				data(bool is_clear); 														//��ȡ����
		void					erase_data(size_t index, size_t len);										//��������
		void*					argument() const;															//��ò���
		void					argument(void* arg);														//���ò���
		const char*				remote_ip();																//��ȡԶ��IP
		unsigned short			remote_port() const; 														//��ȡԶ�̶˿�
		void					close();																	//�ر�
	};

	//�ص�����
	enum IOCallbackType{ kConnect, kAccept, kReceive, kRecycle };

	//TCP�ص�����ָ������
	typedef void(*TCPCallbackFun)(IOCallbackType type, ISession* session, int error_code, void* arg);

	//UDP�ص�����ָ������
	typedef void(*UDPCallbackFun)(const char* data, size_t length, const char* remote_ip, int error_code, void* arg);

	//��ʼ��TCP����(�˿�,������,�ص�����,�ص�����)
	extern "C" NETWORKZ_API bool InitTCPService(unsigned short port, TCPCallbackFun callback, void* arg, size_t concurrence = 500, bool is_print_error_info = true);

	//��ʼ��UDP����(�˿�,��������С,�ص�����,�ص�����,������)
	extern "C" NETWORKZ_API bool InitUDPService(unsigned short port, size_t buffer_size, UDPCallbackFun callback, void* arg, size_t concurrence = 500, bool is_print_error_info = true);

	//������������
	extern "C" NETWORKZ_API void PostConnect(const char* ip, unsigned short port);

	//����
	extern "C" NETWORKZ_API void Run(size_t thread_num);

	//����UDP��Ϣ
	extern "C" NETWORKZ_API void UDPSend(const char* str, size_t length, const char* ip, unsigned short port);

	//�������
	extern "C" NETWORKZ_API void Destroy();

	//��ȡ����IP
	extern "C" NETWORKZ_API void GetHostIPv4(const char**& out_arr_ip, size_t& out_num);
}
