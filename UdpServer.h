#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/lockfree/stack.hpp>

class SharedConstBuffer
{
public:
	// Construct from a std::string.
	explicit SharedConstBuffer(const std::string& data)
		: data_(new std::string(data)),
		buffer_(boost::asio::buffer(data_->data(), data_->size()))
	{
	}

	// Implement the ConstBufferSequence requirements.
	typedef boost::asio::const_buffer value_type;
	typedef const boost::asio::const_buffer* const_iterator;
	const boost::asio::const_buffer* begin() const { return &buffer_; }
	const boost::asio::const_buffer* end() const { return &buffer_ + 1; }

	std::shared_ptr<std::string> data() const {
		return data_;
	}

private:
	std::shared_ptr<std::string> data_;
	boost::asio::const_buffer buffer_;
};


class UdpServer {
public:
	using udp = boost::asio::ip::udp;

	UdpServer() :io_context(), work(io_context) {

	}

	~UdpServer() {
		stop();
	}

	bool listen(unsigned short port, int thread_num, int buffer_size) {
		this->buffer_size = buffer_size;
		try {
			udp::socket socket(io_context, udp::endpoint(udp::v4(), port));
			p_socket = std::make_shared<udp::socket>(std::move(socket));
			local_endpoint = p_socket->local_endpoint();

			async_receive_from();

			for (int i = 0; i < thread_num; i++)
				t_group.create_thread(boost::bind(&boost::asio::io_service::run, &io_context));
			return true;
		}
		catch (boost::system::system_error error) {
			std::cout << "listen error " << error.code() << " " << error.what() << std::endl;
			return false;
		}
	}

	void async_receive_from() {
		std::shared_ptr<char[]> p_data = get_or_create_a_buffer();

		p_socket->async_receive_from(boost::asio::buffer(p_data.get(), buffer_size), remote_endpoint,
			[p_data, this](boost::system::error_code ec, std::size_t bytes_recvd)
			{
				//backup datas before initiate another async_receive_from
				udp::endpoint remote_endpoint_backup = remote_endpoint;

				//call to async_receive_from
				async_receive_from();

				//call handler
				on_receive(ec, remote_endpoint_backup, p_data, bytes_recvd);

				//push back p_data
				while (data_buffer.push(p_data) == false);
			});
	}

	void async_send_to(udp::endpoint remote_endpoint, const std::string& data) {
		SharedConstBuffer scb(data);
		p_socket->async_send_to(scb, remote_endpoint, [=](boost::system::error_code ec, std::size_t bytes_sent) {
			on_async_send_over(remote_endpoint, scb.data(), ec, bytes_sent);
			});
	}

	virtual void on_receive(boost::system::error_code ec, udp::endpoint remote_endpoint, std::shared_ptr<char[]> p_data, size_t bytes_recvd) {
		if (!ec) {
			//std::string str(p_data.get(), bytes_recvd);
			//std::stringstream ss(str);
			std::cout << "received " << bytes_recvd << " bytes " << std::endl;
		}
		else {
			std::cout << "R_ERR " << bytes_recvd << " bytes " << ec.value() << " " << ec.message() << " " << std::endl;
		}
	}

	virtual void on_async_send_over(udp::endpoint remote_endpoint, std::shared_ptr<std::string> p_data, boost::system::error_code ec, std::size_t bytes_sent) {
		std::cout << "sent " << bytes_sent << " bytes" << std::endl;
	}

	void stop() {
		io_context.stop();
		t_group.join_all();
	}

protected:
	boost::asio::io_context io_context;
	boost::asio::io_service::work work;
	std::shared_ptr<udp::socket> p_socket;

	udp::endpoint local_endpoint;
	udp::endpoint remote_endpoint;

	boost::thread_group t_group;
private:
	int buffer_size = 1024;

	boost::lockfree::stack<std::shared_ptr<char[]>> data_buffer;

	std::shared_ptr<char[]> get_or_create_a_buffer() {
		std::shared_ptr<char[]> p;
		if (!data_buffer.pop(p)) {
			p.reset(new char[buffer_size]);
		}
		return p;
	}
};
