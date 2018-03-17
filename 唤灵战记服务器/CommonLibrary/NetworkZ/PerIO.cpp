#include "stdafx.h"
#include "PerIO.h"
#include "ASIO.h"

namespace networkz
{
	CPerIO::CPerIO(CASIO* asio) :
		asio_{ asio },
		tcp_socket_{ asio->io_service() }
	{
		memset(receive_buffer_, 0, sizeof(receive_buffer_));
		memset(ip_buffer_, 0, sizeof(ip_buffer_));
		data_.reserve(kReceiveBufferSize);
	}

	CPerIO::~CPerIO()
	{

	}

	void CPerIO::send(const char* str, size_t len)
	{
		if (str && len)
		{
			tcp_socket_.async_send(boost::asio::buffer(str, len), [this](boost::system::error_code err, size_t num)
			{
				if (err && asio_->tcp_is_print())
				{
					asio_->lock();
					printf("tcp send:%s\n", err.message().c_str());
					asio_->unlock();
				}
			});
		}
		else
		{
			PRINT_MESSAGE("error:发送TCP消息空指针或0长度");
		}
	}

	size_t CPerIO::data_length() const
	{
		return data_.length();
	}

	const char*	CPerIO::data(bool is_clear)
	{
		is_clear_data_ = is_clear;

		return data_.c_str();
	}

	void CPerIO::erase_data(size_t index, size_t len)
	{
		try
		{
			data_.erase(index, len);
		}
		catch (std::exception exc)
		{
			PRINT_MESSAGE(exc.what());
		}
	}

	void* CPerIO::argument() const
	{
		return argument_;
	}

	void CPerIO::argument(void* arg)
	{
		argument_ = arg;
	}

	const char*	CPerIO::remote_ip()
	{
		memset(ip_buffer_, 0, sizeof(ip_buffer_));
		strcpy_s(ip_buffer_, tcp_socket_.remote_endpoint().address().to_string().c_str());

		return ip_buffer_;
	}

	unsigned short CPerIO::remote_port() const
	{
		return tcp_socket_.remote_endpoint().port();
	}

	void CPerIO::close()
	{
		tcp_socket_.close();
	}

	void CPerIO::PostReceive()
	{
		if (false == is_valid_)
			return;

		tcp_socket_.async_read_some(boost::asio::buffer(receive_buffer_, sizeof(receive_buffer_)), [this](boost::system::error_code err, size_t bytes_trans)
		{
			if (!err)
			{
				data_.append(receive_buffer_, bytes_trans);

				memset(receive_buffer_, 0, sizeof(receive_buffer_));

				asio_->DoTCPCallback(IOCallbackType::kReceive, this, err.value());

				if (is_clear_data_)
					data_.clear();

				is_clear_data_ = true;

				PostReceive();
			}
			else
			{
				is_valid_ = false;

				asio_->DoTCPCallback(IOCallbackType::kRecycle, this, err.value());

				if (asio_->tcp_is_print())
				{
					asio_->lock();
					printf("tcp receive:%s\n", err.message().c_str());
					asio_->unlock();
				}

				asio_->DeleteIO(this);
			}
		});
	}
}