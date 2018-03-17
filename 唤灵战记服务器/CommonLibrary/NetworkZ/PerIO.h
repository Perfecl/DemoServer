#pragma once
#include "boost\asio.hpp"

namespace networkz
{
	class CASIO;
	class CPerIO : public ISession
	{
	public:
		static const size_t		kReceiveBufferSize{ 4096 };												//接收缓冲区大小

	public:
		CPerIO(CASIO* asio);
		~CPerIO();

		boost::asio::ip::tcp::socket&	socket(){ return tcp_socket_; }

		void					PostReceive();																//投递接收请求

		void					send(const char* str, size_t len) override;									//发送
		size_t					data_length() const override; 												//获取数据长度
		const char*				data(bool is_clear) override; 												//获取数据
		void					erase_data(size_t index, size_t len) override;								//擦除数据
		void*					argument() const override;													//获得参数
		void					argument(void* arg) override;												//设置参数
		const char*				remote_ip() override;														//获取远程IP
		unsigned short			remote_port() const override; 												//获取远程端口
		void					close() override;															//关闭

	private:
		CASIO*	const					asio_;												//ASIO指针

		boost::asio::ip::tcp::socket	tcp_socket_;										//TCP Socket
		bool							is_valid_{ true };									//是否有效的IO

		char							receive_buffer_[kReceiveBufferSize];				//接收缓冲区缓存
		std::string						data_;												//数据区

		bool							is_clear_data_{ true };								//是否清除数据区

		char							ip_buffer_[40];										//IP缓冲区

		void*							argument_{ nullptr };								//其他参数
	};
}