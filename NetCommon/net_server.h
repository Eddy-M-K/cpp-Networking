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
                try {
                    // Tell ASIO to wait for client connection to prevent 
                    // it from ending immediately when called in a new thread
                    WaitForClientConnection();

                    m_threadContext = std::thread([this]() { m_asioContext.run(); });
                } catch (std::exception &e) {
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
                // Acceptor object provides a unique socket for incoming connection attempt
                // It waits until a socket connects
                m_asioAcceptor.async_accept(
                    [this](std::error_code ec, asio::ip::tcp::socket socket)
                    {
                        if (!ec)
                        {
                            std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";

                            std::shared_ptr<connection<T>> newconn =
                                std::make_shared<connection<T>>(connection<T>::owner::server,
                                    m_asioContext, std::move(socket), m_qMessagesIn);

                            //Give the user server a chance to deny connections
                            if (OnClientConnect(newconn)) {
                                // Connection allowed, so add to container of new connections
                                // If connection is denied, newconn goes out of scope and is deleted (shared_ptr)
                                m_deqConnections.push_back(std::move(newconn));

                                m_deqConnections.back()->ConnectToClient(nIDCounter++);

                                std::cout << "[" << m_deqConnections.back()->GetID() << "] Connection Approved\n";
                            } else {
                                std::cout << "[-----] Connection Denied\n";
                            }
                        } else {
                            // Error has occurred during acceptance
                            std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
                        }

                        // Prime the ASIO context with more work
                        // Simply wait for another connection
                        WaitForClientConnection();
                    });
            }

            // Send a message to a specific client
            void MessageClient(std::shared_ptr<connection<T>> client, const message<T> &msg)
            {
                if (client && client->IsConnected()) {
                    client->Send(msg);
                } else {
                    // Limitation of TCP protocol is that we do not know if client was disconnected
                    // Assume it was disconnected if IsConnected() returns false
                    OnClientDisconnect(client);
                    client.reset();
                    m_deqConnections.erase(
                        std::remove(m_deqConnections.begin(), m_deqConnections.end(), client), m_deqConnections.end());
                }
            }

            // Send message to all clients
            void MessageAllClients(const message<T> &msg, std::shared_ptr<connection<T>> pIgnoreClient = nullptr)
            {
                bool bInvalidClientExists = false;

                for (auto &client : m_deqConnections) {
                    // Check client is connected
                    if (client && client->IsConnected()) {
                        if (client != pIgnoreClient) client->Send(msg);
                    } else {
                        // Assumed that client was disconnected
                        OnClientDisconnect(client);
                        client.reset();
                        bInvalidClientExists = true;
                    }
                }

                if (bInvalidClientExists) m_deqConnections.erase(
                    std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr), m_deqConnections.end());
            }

            void Update(size_t nMaxMessages = -1, bool bWait = false)
            {
                // Prevents server from occupying 100% of a CPU core
                if (bWait) m_qMessagesIn.wait();

                size_t nMessageCount = 0;

                while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty()) {
                    // Grab the front message
                    auto msg = m_qMessagesIn.pop_front();

                    // Pass to message handler
                    OnMessage(msg.remote, msg.msg);
                    
                    nMessageCount++;
                }
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
            virtual void OnMessage(std::shared_ptr<connection<T>> client, message<T> &msg)
            {

            }

            // Called when a client is validated
            virtual void OnClientValidated(std::shared_ptr<connection<T>> client)
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