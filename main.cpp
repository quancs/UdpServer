#include <iostream>
#include <boost/array.hpp>
#include <thread>

#include "UdpServer.h"

using std::cout;
using std::endl;
using boost::asio::ip::udp;

// A reference-counted non-modifiable buffer class.

class B : public UdpServer {
public:
	virtual void on_receive(boost::system::error_code ec, udp::endpoint remote_endpoint, std::shared_ptr<char[]> p_data, size_t bytes_recvd) {
		if (!ec) {
			//std::string str(p_data.get(), bytes_recvd);
			//std::stringstream ss(str);
			std::cout << "received-B " << bytes_recvd << " bytes " << std::endl;
		}
		else {
			std::cout << "R_ERR-B " << bytes_recvd << " bytes " << ec.value() << " " << ec.message() << " " << std::endl;
		}
	}
};


void async_send(UdpServer& server, std::string ip, unsigned short port, std::string msg, int times) {
	udp::endpoint ep(boost::asio::ip::address::from_string(ip), port);
	for (int i = 0; i < times; i++)
		server.async_send_to(ep, msg);
}


int main()
{
	try
	{
		B server1;
		server1.listen(11013, 4, 1024);

		UdpServer server2;
		server2.listen(11014, 4, 1024);

		std::thread t([&]() {
			async_send(server2, std::string("127.0.0.1"), 11013, std::string("Hello world!"), 100);
			});
		std::thread t2([&]() {
			async_send(server2, std::string("127.0.0.1"), 11013, std::string("Hello world!"), 100);
			});
		std::thread t3([&]() {
			async_send(server2, std::string("127.0.0.1"), 11013, std::string("Hello world!"), 100);
			});
		std::thread t4([&]() {
			async_send(server2, std::string("127.0.0.1"), 11013, std::string("Hello world!"), 100);
			});

		t.join();
		t2.join();
		t3.join();
		t4.join();
		std::cout << "over" << endl;
		Sleep(15000);
		//std::cout << "r:" << server1.rcount << " s:" << server2.scount << endl;
		server1.stop();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
