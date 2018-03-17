#pragma once
#include "PerIO.h"
#include <boost\pool\object_pool.hpp>
#include <thread>

namespace networkz
{
	class CASIO
	{
	public:
		static void		InitializeInstance();		//初始化实例
		static void		DeleteInstance();			//删除实例
		static CASIO*	GetInstance();				//获取实例

	private:
		static CASIO*	asio_instance_;				//ASIO实例	

	public:
		~CASIO();

		void			Run(size_t thread_num);																															//运行

		bool			InitUDPService(unsigned short port, size_t buffer_size, UDPCallbackFun callback, void* arg, size_t concurrence, bool is_print_error_info);		//初始化UDP服务
		void			UDPSend(const char* str, size_t length, const char* ip, unsigned short port);																	//发送UDP消息

		bool			InitTCPService(unsigned short port, TCPCallbackFun callback, void* arg, size_t concurrence, bool is_print_error_info);							//初始化TCP服务
		void			DoTCPCallback(IOCallbackType type, CPerIO* perio, int err_code);																				//调用TCP回调函数
		void			PostConnect(const char* ip, unsigned short port);																								//投递连接请求

		CPerIO*			NewIO();																																		//创建一个新IO
		void			DeleteIO(CPerIO* perio);																														//删除一个IO

		inline void		lock(){ EnterCriticalSection(&asio_lock_); }																									//上锁
		inline void		unlock(){ LeaveCriticalSection(&asio_lock_); }																									//解锁

		inline bool							tcp_is_print(){ return tcp_is_print_; }
		inline boost::asio::io_service&		io_service(){ return io_service_; }

	private:
		boost::asio::io_service											io_service_;											//IO服务

		CRITICAL_SECTION												asio_lock_;												//线程锁

		std::vector<std::thread>										threads_;												//所有的线程

		std::unique_ptr<boost::asio::ip::udp::socket>					udp_socket_{ nullptr };									//UDP Socket
		std::vector<std::pair<char*, boost::asio::ip::udp::endpoint>*>	udp_buffers_;											//UDPBuffer
		size_t															udp_buffer_size_{ 0 };									//UDP的缓冲区大小
		UDPCallbackFun													udp_callback_{ nullptr };								//UDP回调函数
		void*															udp_callback_argument_{ nullptr };						//UDP回调参数
		bool															udp_is_print_{ false };									//是否打印UDP错误

		std::unique_ptr<boost::asio::ip::tcp::acceptor>					tcp_acceptor_{ nullptr };								//TCP连接接收器
		TCPCallbackFun													tcp_callback_{ nullptr };								//UDP回调函数
		void*															tcp_callback_argument_{ nullptr };						//UDP回调参数
		bool															tcp_is_print_{ false };									//是否打印TCP错误

		boost::object_pool<CPerIO>										perio_pool_;											//IO内存池

		CASIO();

		void	__PostUDPReceive(std::pair<char*, boost::asio::ip::udp::endpoint>& pair);										//投递UDP接收请求

		void	__PostTCPAccept(CPerIO* perio);
	};
}
