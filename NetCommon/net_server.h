#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"
#include "net_connection.h"

namespace kim
{
  namespace net
  {
    template<typename T>
    class server_interface
    {
    public:
      // Server listens to connections on the endpoint and looks for IPv4 addresses
      server_interface(uint16_t port)
        : m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
      {
        
      }

      virtual ~server_interface()
      {
        Stop();
      }

      bool Start()
      {
        try
        {
          // Tell ASIO to wait for client connection to prevent 
          // it from ending immediately when called in a new thread
          WaitForClientConnection();

          m_threadContext = std::thread([this]() { m_asioContext.run(); });
        }
        catch (std::exception& e)
        {
          // Something prohibited the server from listening
          std::cerr << "[SERVER] Exception: " << e.what() << "\n";
          return false;
        }

        std::cout << "[SERVER] Started!\n";
        return true;
      }

      void Stop()
      {
        // Request the context to close
        m_asioContext.stop();

        // Tidy up the context thread;
        if (m_threadContext.joinable()) m_threadContext.join();

        std::cout << "[SERVER] Stopped!\n";
      }

      // Asynchronous - instruct ASIO to wait for connection
      void WaitForClientConnection()
      {
        m_asioAcceptor.async_accept_accept(
          [this](std::error_code ec, asio::ip::tcp::socket socket)
          {
            if (!ec)
            {
              std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";

              std::shared_ptr<connection<T>> newconn =
                std::make_shared<connection<T>>(connection<T>::owner::server,
                  m_asioContext, std::move(socket), m_qMessagesIn);

              // Give the user server a chance to deny connections
              if (OnClientConnect(newconn))
              {

              }
              else
              {
                std::cout << "[-----] Connection Denied\n";
              }
            }
            else
            {
              // Error has occurred during acceptance
              std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
            }

            // Prime the ASIO context with more work
            // Simply wait for another connection
            WaitForClientConnection();
          });
      }

      // Send a message to a specific client
      void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg)
      {

      }

      // Send message to all clients
      void MessageAllClients(const message<T>& msg, std::shared_ptr<connection<T>> pIgnoreClient = nullptr)
      {

      }

    protected:
      // Called when a client connects, you can veto the connection by returning false
      virtual bool OnClientConnect(std::shared_ptr<connection<T>> client)
      {
        return false;
      }

      // Called when a client appears to have disconnected
      virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client)
      {

      }

      // Called when a message arrives 
      virtual void OnMessage(std::shared_ptr<connection<T>> client, message<T>& msg)
      {

      }

      // Thread safe queue for incoming message packets
      tsqueue<owned_message<T>> m_qMessagesIn;

      // Container of active and validated connections
      std::deque<std::shared_ptr<connection<T>>> m_deqConnections;

      // Order of declaration is imporant - it is also the order of initialization
      asio::io_context m_asioContext;
      std::thread m_threadContext;

      // These things need an ASIO context
      asio::ip::tcp::acceptor m_asioAcceptor;

      // Clients will be identified in the "wider system" via an ID
      uint32_t nIDCounter = 10000;


    };
  }
}