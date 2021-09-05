#pragma once
#include "net_common.h"
#include "net_message.h"
#include "net_tsqueue.h"
#include "net_connection.h"

namespace kim
{
  namespace net
  {
    // Responsible for setting up ASIO and the connection
    // Access point to talk to the server
    template <typename T>
    class client_interface
    {
    public:
      client_interface() : m_socket(m_context)
      {
        // Initialize the socket with the IO context, so it can function
      }

      virtual ~client_interface()
      {
        // If the client is destroyed, always try and disconnect from server
        Disconnect();
      }

      // Connect to server with hostname/IP-address and port
      bool Connect(const std::string& host, const uint16_t port)
      {
        try
        {
          // Create connection
          m_connection = std::make_unique<connection<T>>();

          // Resolve hostname/IP-address into tangible, physical address
          asio::ip::tcp::resolver resolver(m_context);
          m_endpoints = resolver.resolve(host, std::to_string(port));

          // Tell the connection object to connect to server
          m_connection->ConnectToServer(m_endpoints);

          // Start Context Thread
          thrContext = std::thread([this]() { m_context.run(); });
        }
        catch (std::exception& e)
        {
          std::cerr << "Client Exception: " << e.what() << "\n";
          return false;
        }

        return false;
      }

      // Disconnect from server
      void Disconnect()
      {
        // If connection exists, and it's connected:
        if (IsConnected())
        {
          // Disconnect from server gracefully
          m_connection->Disconnect();
        }

        // Done with ASIO context
        m_context.stop();
        // Done with thread
        if (thrContext.joinable()) thrContext.join();

        // Destroy the connection object
        m_connection.release();
      }

      // Check if client is actually connected to a server
      bool IsConnected()
      {
        if (m_connection) return m_connection->IsConnected();
        else return false;
      }

      // Retireve queue of messages from server
      tsqueue<owned_message<T>>& Incoming()
      {
        return m_qMessagesIn;
      }

    protected:
      // ASIO context handles the data transfer
      asio::io_context m_context;
      // ASIO context needs a thread of its own to execute its work commands
      std::thread thrContext;
      // Hardware socket that is connected to the server
      asio::ip::tcp::socket m_socket;
      // Client has a single instance of a "connection" object, which handles data transfer
      std::unique_ptr<connection<T>> m_connection;

    private:
      // This is the thread safe queue of incoming messages from server
      tsqueue<owned_message<T>> m_qMessagesIn;
    };
  }
}