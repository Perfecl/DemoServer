#include "stdafx.h"
#include "ASIO.h"

namespace networkz
{
	CASIO* CASIO::asio_instance_{ nullptr };

	void CASIO::InitializeInstance()
	{
		if (nullptr == asio_instance_)
			asio_instance_ = new CASIO;
	}

	void CASIO::DeleteInstance()
	{
		delete asio_instance_;
		asio_instance_ = nullptr;
	}

	CASIO* CASIO::GetInstance()
	{
		return asio_instance_;
	}

	CASIO::CASIO()
	{
		InitializeCriticalSectionAndSpinCount(&asio_lock_, 4000);
	}

	CASIO::~CASIO()
	{
		io_service_.stop();

		for (auto &it : threads_)
			it.join();

		for (auto &it : udp_buffers_)
		{
			delete[]it->first;
			delete it;
		}

		DeleteCriticalSection(&asio_lock_);
	}

	void CASIO::Run(size_t thread_num)
	{
		if (0 == thread_num)
		{
			PRINT_MESSAGE("warning:运行线程数为0,自动切换成1");
			thread_num = 1;
		}

		for (size_t i = 0; i < thread_num; i++)
		{
			threads_.push_back(std::thread([this]()
			{
				try
				{
					io_service_.run();
				}
				catch (std::exception exc)
				{
					PRINT_MESSAGE(exc.what());
				}
				catch (...)
				{
					PRINT_MESSAGE("未知的异常错误");
				}
			}));
		}
	}

	bool CASIO::InitUDPService(unsigned short port, size_t buffer_size, UDPCallbackFun callback, void* arg, size_t concurrence, bool is_print_error_info)
	{
		if (udp_socket_)
		{
			PRINT_MESSAGE("error:初始化UDP服务失败：已初始化");
			return false;
		}

		try
		{
			udp_socket_ = std::make_unique<boost::asio::ip::udp::socket>(io_service_, std::move(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port)));

			if (0 == buffer_size)
			{
				udp_buffer_size_ = 1;
				PRINT_MESSAGE("warning:UDP缓冲区设置太小,已自动调整为1Byte");
			}
			else if (buffer_size > 65536)
			{
				udp_buffer_size_ = 65536;
				PRINT_MESSAGE("warning:UDP缓冲区设置太大,已自动调整为65536Bytes");
			}
			else
			{
				udp_buffer_size_ = buffer_size;
			}
		}
		catch (std::exception exc)
		{
			udp_socket_ = nullptr;
			udp_buffer_size_ = 0;
			PRINT_MESSAGE(exc.what());
			return false;
		}
		catch (...)
		{
			udp_socket_ = nullptr;
			udp_buffer_size_ = 0;
			PRINT_MESSAGE("error:创建UDP socket错误");
			return false;
		}

		udp_is_print_ = is_print_error_info;
		udp_callback_ = callback;
		udp_callback_argument_ = arg;

		for (size_t i = 0; i < concurrence; i++)
		{
			udp_buffers_.push_back(new std::pair<char*, boost::asio::ip::udp::endpoint>(new char[udp_buffer_size_]{0}, std::move(boost::asio::ip::udp::endpoint{})));
			__PostUDPReceive(*udp_buffers_.back());
		}

		return true;
	}

	void CASIO::UDPSend(const char* str, size_t length, const char* ip, unsigned short port)
	{
		if (nullptr == udp_socket_)
		{
			PRINT_MESSAGE("发送UDP消息错误:请先初始化UDP服务");
			return;
		}

		if (nullptr == str)
		{
			PRINT_MESSAGE("发送UDP消息错误:字符串空指针");
			return;
		}

		if (nullptr == ip)
		{
			PRINT_MESSAGE("发送UDP消息错误:IP空指针");
			return;
		}

		udp_socket_->async_send_to(boost::asio::buffer(str, length), boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(ip), port), [this](boost::system::error_code err, std::size_t bytes_sen)
		{
			if (err && udp_is_print_)
			{
				lock();
				printf("udpsend:%s\n", err.message().c_str());
				unlock();
			}
		});
	}

	bool CASIO::InitTCPService(unsigned short port, TCPCallbackFun callback, void* arg, size_t concurrence, bool is_print_error_info)
	{
		if (tcp_acceptor_)
		{
			PRINT_MESSAGE("error:初始化TCP服务失败：已初始化");
			return false;
		}

		try
		{
			tcp_acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(io_service_, std::move(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)));
		}
		catch (std::exception exc)
		{
			tcp_acceptor_ = nullptr;
			PRINT_MESSAGE(exc.what());
			return false;
		}
		catch (...)
		{
			tcp_acceptor_ = nullptr;
			PRINT_MESSAGE("error:创建TCP acceptor错误");
			return false;
		}

		tcp_is_print_ = is_print_error_info;
		tcp_callback_ = callback;
		tcp_callback_argument_ = arg;

		for (size_t i = 0; i < concurrence; i++)
			__PostTCPAccept(NewIO());

		return true;
	}

	void CASIO::PostConnect(const char* ip, unsigned short port)
	{
		if (nullptr == ip)
		{
			PRINT_MESSAGE("发送TCP连接请求失败:IP空指针");
			return;
		}

		if (nullptr == tcp_acceptor_)
		{
			PRINT_MESSAGE("发送TCP连接请求失败:请先初始化TCP服务");
			return;
		}

		CPerIO* perio = NewIO();

		perio->socket().async_connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip), port), [this, ip, port, perio](boost::system::error_code err)
		{
			DoTCPCallback(IOCallbackType::kConnect, perio, err.value());

			if (!err)
			{
				perio->PostReceive();
			}
			else
			{
				if (tcp_is_print_)
				{
					lock();
					printf("tcp connect:%s\n", err.message().c_str());
					unlock();
				}

				DeleteIO(perio);
			}
		});
	}

	CPerIO*	CASIO::NewIO()
	{
		lock();

		CPerIO* perio{ nullptr };

		try
		{
			perio = perio_pool_.construct(this);
		}
		catch (...)
		{
			perio = nullptr;
			PRINT_MESSAGE("error:IO内存分配失败");
		}

		unlock();

		return perio;
	}

	void CASIO::DeleteIO(CPerIO* perio)
	{
		if (nullptr == perio)
			return;

		lock();

		try
		{
			perio_pool_.destroy(perio);
		}
		catch (...)
		{
			PRINT_MESSAGE("error:IO内存回收失败");
		}

		unlock();
	}

	void CASIO::DoTCPCallback(IOCallbackType type, CPerIO* perio, int err_code)
	{
		if (tcp_callback_)
			tcp_callback_(type, perio, err_code, tcp_callback_argument_);
	}

	void CASIO::__PostUDPReceive(std::pair<char*, boost::asio::ip::udp::endpoint>& pair)
	{
		if (io_service_.stopped())
		{
			return;
		}
		else if (nullptr == udp_socket_)
		{
			PRINT_MESSAGE("error:udpsocket空指针");
			return;
		}
		else
		{
			udp_socket_->async_receive_from(boost::asio::buffer(pair.first, udp_buffer_size_), pair.second, [this, &pair](boost::system::error_code err, std::size_t bytes_recvd)
			{
				if (udp_callback_)
					udp_callback_(pair.first, bytes_recvd, pair.second.address().to_string().c_str(), err.value(), udp_callback_argument_);

				if (err && udp_is_print_)
				{
					lock();
					printf("udp receive:%s\n", err.message().c_str());
					unlock();
				}

				__PostUDPReceive(pair);
			});
		}
	}

	void CASIO::__PostTCPAccept(CPerIO* perio)
	{
		if (nullptr == perio)
		{
			return;
		}
		else if (io_service_.stopped())
		{
			DeleteIO(perio);
			return;
		}
		else if (nullptr == tcp_acceptor_)
		{
			DeleteIO(perio);
			PRINT_MESSAGE("error:tcp_acceptor_空指针");
			return;
		}
		else
		{
			tcp_acceptor_->async_accept(perio->socket(), [this, perio](boost::system::error_code err)
			{
				DoTCPCallback(IOCallbackType::kAccept, perio, err.value());

				if (!err)
				{
					perio->PostReceive();
					__PostTCPAccept(NewIO());
				}
				else
				{
					if (tcp_is_print_)
					{
						lock();
						printf("tcp accept:%s\n", err.message().c_str());
						unlock();
					}

					__PostTCPAccept(perio);
				}
			});
		}
	}
}