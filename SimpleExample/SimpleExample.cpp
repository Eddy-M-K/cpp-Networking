// Asynchronous part of ASIO means reading data not all at once
// since it's difficult to predict when data will be received
#include <iostream>
#include <chrono>

// Tell program to use a specific version of Windows 
// since each verison of Windows has differences in its networking
#ifdef _WIN32
// Use Windows 10
#define _WIN32_WINNT 0x0A00
#endif 

// Do not use boost framework
#define ASIO_STANDALONE

#include <asio.hpp>
// Header file for movement of memory
#include <asio/ts/buffer.hpp>
// Header file for network communication
#include <asio/ts/internet.hpp>

// Buffer to store received data and display all at once later
std::vector<char> vBuffer(20 * 1024);

void GrabSomeData(asio::ip::tcp::socket &socket)
{
	socket.async_read_some(asio::buffer(vBuffer.data(), vBuffer.size()),
		[&](std::error_code ec, std::size_t length)
		{
			if (!ec) {
				std::cout << "\n\nRead " << length << " bytes\n\n";

				for (int i = 0; i < length; i++) {
					std::cout << vBuffer[i];
				}

				GrabSomeData(socket);

			}
		});

}

int main() 
{
	// Variable for errors
	asio::error_code ec;

	// Create a "context" - essentially the platform specific interface
	asio::io_context context;

	// Give fake tasks to ASIO so the context doesn't finish
	asio::io_context::work idleWork(context);

	// Start the context in its own thread in its own space
	// without blocking the main proram
	std::thread thrContext = std::thread([&]() { context.run(); });

	/*	Get the address of somewhere we wish to connect to
			Use TCP endpoint, which is defined by an IP address
			String of IP address is converted
			Errors are assigned to ec
			Use TCP port 80 (HTTP)																
	*/
	asio::ip::tcp::endpoint endpoint(asio::ip::make_address("51.38.81.49", ec), 80);

	// Create a socket, the context will deliver the implementation
	asio::ip::tcp::socket socket(context);

	// Tell socket to try and connect
	socket.connect(endpoint, ec);

	if (!ec) {
		std::cout << "Connected!" << std::endl;
	}
	else {
		std::cout << "Failed to connect to address:\n" << ec.message() << std::endl;
	}

	// is_open returns true if connection was successfully established
	if (socket.is_open()) {
		// Prime context to read bytes before sending data
		GrabSomeData(socket);

		// HTTP request (sends text strings)
		std::string sRequest =
			"GET /index.html HTTP/1.1\r\n"
			"Host: david-barr.co.uk\r\n"
			"Connection: close\r\n\r\n";

		// write_some means send as much info as possible
		// buffer is an array of bytes
		socket.write_some(asio::buffer(sRequest.data(), sRequest.size()), ec);

		// Program does something else, while ASIO handles data transfer in background
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(2000ms);

		//// make socket wait for server to respond and then read
		//socket.wait(socket.wait_read);

		//// Check if bytes to read are available
		//size_t bytes = socket.available();
		//std::cout << "Bytes Available: " << bytes << std::endl;

		//if (bytes > 0) {
		//	std::vector<char> vBuffer(bytes);
		//	socket.read_some(asio::buffer(vBuffer.data(), vBuffer.size()), ec);

		//	for (auto c : vBuffer) {
		//		std::cout << c;
		//	}
	}

	system("pause");
	return 0;
}
