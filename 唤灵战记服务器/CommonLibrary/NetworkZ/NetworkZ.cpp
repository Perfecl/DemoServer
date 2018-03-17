#include "stdafx.h"
#include "NetworkZ.h"
#include "ASIO.h"

namespace networkz
{
	const char**	host_ip_v4{ nullptr };
	size_t			host_up_v4_num{ 0 };

	NETWORKZ_API bool InitUDPService(unsigned short port, size_t buffer_size, UDPCallbackFun callback, void* arg, size_t concurrence, bool is_print_error_info)
	{
		CASIO::InitializeInstance();
		CASIO* asio = CASIO::GetInstance();

		if (nullptr == asio)
		{
			PRINT_MESSAGE("error:ASIO实例初始化错误");
			return false;
		}
		else
		{
			return asio->InitUDPService(port, buffer_size, callback, arg, concurrence, is_print_error_info);
		}
	}

	NETWORKZ_API bool InitTCPService(unsigned short port, TCPCallbackFun callback, void* arg, size_t concurrence, bool is_print_error_info)
	{
		CASIO::InitializeInstance();
		CASIO* asio = CASIO::GetInstance();

		if (nullptr == asio)
		{
			PRINT_MESSAGE("error:ASIO实例初始化错误");
			return false;
		}
		else
		{
			return asio->InitTCPService(port, callback, arg, concurrence, is_print_error_info);
		}
	}

	NETWORKZ_API void UDPSend(const char* str, size_t length, const char* ip, unsigned short port)
	{
		CASIO* asio = CASIO::GetInstance();

		if (nullptr == asio)
		{
			PRINT_MESSAGE("error:发送UDP消息错误:未初始化ASIO实例");
			return;
		}
		else
		{
			asio->UDPSend(str, length, ip, port);
		}
	}

	NETWORKZ_API void PostConnect(const char* ip, unsigned short port)
	{
		CASIO* asio = CASIO::GetInstance();

		if (nullptr == asio)
		{
			PRINT_MESSAGE("error:发送Connect消息错误:未初始化ASIO实例");
			return;
		}
		else
		{
			asio->PostConnect(ip, port);
		}
	}

	NETWORKZ_API void Run(size_t thread_num)
	{
		CASIO* asio = CASIO::GetInstance();

		if (nullptr == asio)
		{
			PRINT_MESSAGE("error:网络组件运行错误:未初始化ASIO实例");
			return;
		}
		else
		{
			asio->Run(thread_num);
		}
	}

	NETWORKZ_API void Destroy()
	{
		CASIO::DeleteInstance();
		for (size_t i = 0; i < host_up_v4_num; i++)
			delete[] host_ip_v4[i];
		delete[] host_ip_v4;
		host_ip_v4 = nullptr;
		host_up_v4_num = 0;
	}

	NETWORKZ_API void GetHostIPv4(const char**& out_arr_ip, size_t& out_num)
	{
		if (nullptr == host_ip_v4)
		{
			std::vector<std::string> vct_host_ip_v4;

			boost::asio::io_service	io_service;
			boost::asio::ip::tcp::resolver resolver{ io_service };
			boost::asio::ip::tcp::resolver::query	query(boost::asio::ip::host_name(), "");
			boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
			boost::asio::ip::tcp::resolver::iterator end;

			while (iter != end)
			{
				boost::asio::ip::tcp::endpoint ep = *iter++;
				if (ep.address().is_v4())
					vct_host_ip_v4.push_back(std::move(ep.address().to_string()));
			}

			host_up_v4_num = vct_host_ip_v4.size();
			host_ip_v4 = new const char*[host_up_v4_num];

			for (size_t i = 0; i < host_up_v4_num; i++)
			{
				host_ip_v4[i] = new char[16]{0};
				memcpy_s(const_cast<char*>(host_ip_v4[i]), 16, vct_host_ip_v4[i].c_str(), 15);
			}
		}
		out_arr_ip = host_ip_v4;
		out_num = host_up_v4_num;
	}
}
