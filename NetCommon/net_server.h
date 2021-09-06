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
          WaitForClientConnection();
        }
        catch (std::exception& e)
        {
          // Something prohibited the server from listening
          std::cerr << "[SERVER] Exception: " << e.what() << "\n";
          return false;
        }
      }

      void Stop()
      {

      }

      // Asynchronous - instruct ASIO to wait for connection
      void WaitForClientConnection()
      {

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