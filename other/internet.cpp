#include <iostream>
#include <cstddef>
#include <string>
#include <memory>

#define _WIN32_WINNT 0x0A00
#pragma warning(push, 0)
#include <boost\asio.hpp>
#include <boost\bind\bind.hpp>
#pragma warning(pop)

void client() {
	boost::asio::io_service io_service{};
	while (true) {
		boost::asio::ip::tcp::socket socket{ io_service };
		std::string address{};
		std::cout << "輸入伺服器位址：";
		std::cin >> address;
		socket.connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), 1));
		char send[]{ "來自客戶端的訊息" };
		boost::asio::write(socket, boost::asio::buffer(send, sizeof(send)));
		char receive[sizeof("來自客戶端的訊息")]{};
		boost::asio::read(socket, boost::asio::buffer(receive, sizeof("來自客戶端的訊息")));
		std::cout << receive << '\n';
		socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
		socket.close();
	}
}

void receive_handler(const boost::system::error_code& err, std::size_t a, char* message) {
	std::cout << message << '\n';
}

void send_handler(const boost::system::error_code& err, std::size_t a) {}

boost::asio::io_service io_service{};
boost::asio::ip::tcp::acceptor acceptor{
	io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 1) };

void accept();

void accept_handler(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
	auto receive{ new char[sizeof("來自伺服器的訊息")] };
	boost::asio::async_read(*socket, boost::asio::buffer(receive, sizeof("來自伺服器的訊息")),
		boost::bind(receive_handler, boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred, receive));
	char send[]{ "來自伺服器的訊息" };
	boost::asio::async_write(*socket, boost::asio::buffer(send, sizeof(send)),
		boost::bind(send_handler, boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
	accept();
}

void accept() {
	auto socket{ std::make_shared<boost::asio::ip::tcp::socket>(io_service) };
	acceptor.async_accept(*socket, boost::bind(accept_handler, socket));
}

void server() {
	accept();
	io_service.run();
}

int main() {
	char mode{};
	std::cout << "選擇一個模式：";
	std::cin >> mode;
	switch (mode) {
	case 'C':
	case 'c':
		client();
		break;
	case 'S':
	case 's':
		server();
		break;
	default:
		return -1;
	}
	return 0;
}