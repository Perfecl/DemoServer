#pragma once
#include "PerIO.h"
#include <boost\pool\object_pool.hpp>
#include <thread>

namespace networkz
{
	class CASIO
	{
	public:
		static void		InitializeInstance();		//��ʼ��ʵ��
		static void		DeleteInstance();			//ɾ��ʵ��
		static CASIO*	GetInstance();				//��ȡʵ��

	private:
		static CASIO*	asio_instance_;				//ASIOʵ��	

	public:
		~CASIO();

		void			Run(size_t thread_num);																															//����

		bool			InitUDPService(unsigned short port, size_t buffer_size, UDPCallbackFun callback, void* arg, size_t concurrence, bool is_print_error_info);		//��ʼ��UDP����
		void			UDPSend(const char* str, size_t length, const char* ip, unsigned short port);																	//����UDP��Ϣ

		bool			InitTCPService(unsigned short port, TCPCallbackFun callback, void* arg, size_t concurrence, bool is_print_error_info);							//��ʼ��TCP����
		void			DoTCPCallback(IOCallbackType type, CPerIO* perio, int err_code);																				//����TCP�ص�����
		void			PostConnect(const char* ip, unsigned short port);																								//Ͷ����������

		CPerIO*			NewIO();																																		//����һ����IO
		void			DeleteIO(CPerIO* perio);																														//ɾ��һ��IO

		inline void		lock(){ EnterCriticalSection(&asio_lock_); }																									//����
		inline void		unlock(){ LeaveCriticalSection(&asio_lock_); }																									//����

		inline bool							tcp_is_print(){ return tcp_is_print_; }
		inline boost::asio::io_service&		io_service(){ return io_service_; }

	private:
		boost::asio::io_service											io_service_;											//IO����

		CRITICAL_SECTION												asio_lock_;												//�߳���

		std::vector<std::thread>										threads_;												//���е��߳�

		std::unique_ptr<boost::asio::ip::udp::socket>					udp_socket_{ nullptr };									//UDP Socket
		std::vector<std::pair<char*, boost::asio::ip::udp::endpoint>*>	udp_buffers_;											//UDPBuffer
		size_t															udp_buffer_size_{ 0 };									//UDP�Ļ�������С
		UDPCallbackFun													udp_callback_{ nullptr };								//UDP�ص�����
		void*															udp_callback_argument_{ nullptr };						//UDP�ص�����
		bool															udp_is_print_{ false };									//�Ƿ��ӡUDP����

		std::unique_ptr<boost::asio::ip::tcp::acceptor>					tcp_acceptor_{ nullptr };								//TCP���ӽ�����
		TCPCallbackFun													tcp_callback_{ nullptr };								//UDP�ص�����
		void*															tcp_callback_argument_{ nullptr };						//UDP�ص�����
		bool															tcp_is_print_{ false };									//�Ƿ��ӡTCP����

		boost::object_pool<CPerIO>										perio_pool_;											//IO�ڴ��

		CASIO();

		void	__PostUDPReceive(std::pair<char*, boost::asio::ip::udp::endpoint>& pair);										//Ͷ��UDP��������

		void	__PostTCPAccept(CPerIO* perio);
	};
}
