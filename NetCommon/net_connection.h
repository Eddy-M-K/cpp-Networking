#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"

namespace kim
{
  namespace net
  {
    template <typename T>
    class connection : public std::enable_shared_from_this<connection<T>>
    {
    public:
      connection()
      {}

      virtual ~connection()
      {}

      // Only called by clients
      bool ConnectToServer();
      // Can be called by both clients and servers
      bool Disconnect();
      // Is connection open and active
      bool IsConnected() const;

      bool Send(const message<T>& msg);

    protected:
      // Each connection has a unique socket to a remote
      asio::ip::tcp::socket m_socket;

      // This context is shared with the whole ASIO instance
      asio::io_context& m_asioContext;

      // This queue holds all messages to be sent to the 
      // remote side of this connection
      tsqueue<message<T>> m_qMessagesOut;

      // This queue holds all messages that have been received from
      // the remote side of this connection. Note it is a reference
      // as the "owner" of this connection is expected to provide a queue
      tsqueue<owned_message>& m_qMessagesIn;
    };
  }
}