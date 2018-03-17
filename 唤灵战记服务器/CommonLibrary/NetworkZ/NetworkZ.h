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
		void					send(const char* str, size_t len);											//发送
		size_t					data_length() const; 														//获取数据长度
		const char*				data(bool is_clear); 														//获取数据
		void					erase_data(size_t index, size_t len);										//擦除数据
		void*					argument() const;															//获得参数
		void					argument(void* arg);														//设置参数
		const char*				remote_ip();																//获取远程IP
		unsigned short			remote_port() const; 														//获取远程端口
		void					close();																	//关闭
	};

	//回调类型
	enum IOCallbackType{ kConnect, kAccept, kReceive, kRecycle };

	//TCP回调函数指针类型
	typedef void(*TCPCallbackFun)(IOCallbackType type, ISession* session, int error_code, void* arg);

	//UDP回调函数指针类型
	typedef void(*UDPCallbackFun)(const char* data, size_t length, const char* remote_ip, int error_code, void* arg);

	//初始化TCP服务(端口,并发量,回调函数,回调参数)
	extern "C" NETWORKZ_API bool InitTCPService(unsigned short port, TCPCallbackFun callback, void* arg, size_t concurrence = 500, bool is_print_error_info = true);

	//初始化UDP服务(端口,缓冲区大小,回调函数,回调参数,并发量)
	extern "C" NETWORKZ_API bool InitUDPService(unsigned short port, size_t buffer_size, UDPCallbackFun callback, void* arg, size_t concurrence = 500, bool is_print_error_info = true);

	//发送连接请求
	extern "C" NETWORKZ_API void PostConnect(const char* ip, unsigned short port);

	//运行
	extern "C" NETWORKZ_API void Run(size_t thread_num);

	//发送UDP消息
	extern "C" NETWORKZ_API void UDPSend(const char* str, size_t length, const char* ip, unsigned short port);

	//销毁组件
	extern "C" NETWORKZ_API void Destroy();

	//获取本机IP
	extern "C" NETWORKZ_API void GetHostIPv4(const char**& out_arr_ip, size_t& out_num);
}
