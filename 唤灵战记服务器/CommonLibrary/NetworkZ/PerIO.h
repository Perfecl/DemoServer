#pragma once
#include "boost\asio.hpp"

namespace networkz
{
	class CASIO;
	class CPerIO : public ISession
	{
	public:
		static const size_t		kReceiveBufferSize{ 4096 };												//���ջ�������С

	public:
		CPerIO(CASIO* asio);
		~CPerIO();

		boost::asio::ip::tcp::socket&	socket(){ return tcp_socket_; }

		void					PostReceive();																//Ͷ�ݽ�������

		void					send(const char* str, size_t len) override;									//����
		size_t					data_length() const override; 												//��ȡ���ݳ���
		const char*				data(bool is_clear) override; 												//��ȡ����
		void					erase_data(size_t index, size_t len) override;								//��������
		void*					argument() const override;													//��ò���
		void					argument(void* arg) override;												//���ò���
		const char*				remote_ip() override;														//��ȡԶ��IP
		unsigned short			remote_port() const override; 												//��ȡԶ�̶˿�
		void					close() override;															//�ر�

	private:
		CASIO*	const					asio_;												//ASIOָ��

		boost::asio::ip::tcp::socket	tcp_socket_;										//TCP Socket
		bool							is_valid_{ true };									//�Ƿ���Ч��IO

		char							receive_buffer_[kReceiveBufferSize];				//���ջ���������
		std::string						data_;												//������

		bool							is_clear_data_{ true };								//�Ƿ����������

		char							ip_buffer_[40];										//IP������

		void*							argument_{ nullptr };								//��������
	};
}