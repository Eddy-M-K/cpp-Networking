#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"

namespace kim
{
    namespace net
    {
        template<typename T>
        class server_interface;

        template<typename T>
        class connection : public std::enable_shared_from_this<connection<T>>
        {
        public:
            enum class owner
            {
                server,
                client
            };

            connection(owner parent, asio::io_context &asioContext, asio::ip::tcp::socket socket, tsqueue<owned_message<T>> &qIn)
                : m_asioContext(asioContext), m_socket(std::move(socket)), m_qMessagesIn(qIn)
            {
                m_nOwnerType = parent;

                // Construct validation check data
                if (m_nOwnerType == owner::server) {
                    // Connection is Server -> Client so construct random data for the 
                    m_nHandshakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());

                    // Pre-calculate the result for checking when the client responds
                    m_nHandshakeCheck = scramble(m_nHandshakeOut);
                } else {
                    // Connection is Client -> Server
                    m_nHandshakeIn = 0;
                    m_nHandshakeOut = 0;
                }
            }
            
            virtual ~connection()
            {
            
            }

            uint32_t GetID() const
            {
                return id;
            }

            void ConnectToClient(kim::net::server_interface<T> *server, uint32_t uid = 0)
            {
                if (m_nOwnerType == owner::server) {
                    if (m_socket.is_open()) {
                        id = uid;
                        // Write out the handshake data to be validated
                        WriteValidation();

                        // Wait asynchronously for the validation data
                        ReadValidation(server);
                    }
                }
            }

            // Only called by clients
            void ConnectToServer(const asio::ip::tcp::resolver::results_type &endpoints)
            {
                // Only clients can connect to servers
                if (m_nOwnerType == owner::client) {
                    // Request that ASIO attempt to connect to an endpoint
                    asio::async_connect(m_socket, endpoints,
                        [this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
                        {
                            // Send packet to be validated
                            ReadValidation();
                        });
                }
            }

            // Can be called by both clients and servers
            void Disconnect()
            {
                if (IsConnected()) asio::post(m_asioContext, [this]() { m_socket.close(); });
            }
            // Is connection open and active
            bool IsConnected() const
            {
                return m_socket.is_open();
            }

            // Prime the connection to wait for incoming messages
            void StartListening()
            {

            }

            // Async: Send a message on a one-on-one connection with the server
            void Send(const message<T> &msg)
            {
                // Send a job to ASIO context whenever needed
                asio::post(m_asioContext,
                    [this, msg]()
                    {
                        // If there are outgoing messages in queue, then in the background, ASIO is sending
                        bool bWritingMessage = !m_qMessagesOut.empty();
                        m_qMessagesOut.push_back(msg);
                        if (!bWritingMessage) {
                            WriteHeader();
                        }
                    });
            }

        private: 
            // Async - Prime context to write a message header
            // The outgoing message queue has at least one message to write 
            // so allocate a buffer to hold teh message and then send the bytes
            void WriteHeader()
            {
                asio::async_write(m_socket, asio::buffer(&m_qMessagesOut.front().header, sizeof(message_header<T>)),
                    [this](std::error_code ec, std::size_t length)
                    {
                        if (!ec) {
                            if (m_qMessagesOut.front().body.size() > 0) {
                                WriteBody();
                            } else {
                                m_qMessagesOut.pop_front();

                                // Check if queue is empty
                                if (!m_qMessagesOut.empty())
                                {
                                    WriteHeader();
                                }
                            }
                        } else {
                            std::cout << "[" << id << "] Write Header Fail.\n";
                            m_socket.close();
                        }
                    });
            }

            // Async - Prime context to write a message body
            // A header has been sent and that header indicated a body was needed
            // Fill a transmission buffer with the data and send it
            void WriteBody()
            {
                asio::async_write(m_socket, asio::buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()),
                    [this](std::error_code ec, std::size_t length)
                    {
                        if (!ec) {
                            m_qMessagesOut.pop_front();

                            // Check if queue is empty
                            if (!m_qMessagesOut.empty()) {
                                WriteHeader();
                            }
                        } else {
                            std::cout << "[" << id << "] Write Body Fail.\n";
                            m_socket.close();
                        }
                    });
            }

            // Async - Prime context ready to read a message header (Client has sent a message)
            // ASIO waits until it receives enough bytes to create a header of a message
            // Construct a temporary message object to hold the message header
            void ReadHeader()
            {
                asio::async_read(m_socket, asio::buffer(&m_msgTemporaryIn.header, sizeof(message_header<T>)),
                    [this](std::error_code ec, std::size_t length)
                    {
                        if (!ec) {
                            // Complete message header has been read
                            if (m_msgTemporaryIn.header.size > 0) {
                                // Message has body
                                m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);
                                ReadBody();
                            } else {
                                AddToIncomingMessageQueue();
                            }
                        } else {
                            std::cout << "[" << id << "] Read Header Fail.\n";
                            m_socket.close();
                        }
                    });
            }

            // Async - Prime context ready to read a message body
            // Header requested body and the space for a body has already been allocated
            // int a temporary message object
            void ReadBody()
            {
                asio::async_read(m_socket, asio::buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()),
                    [this](std::error_code ec, std::size_t length)
                    {
                        if (!ec) {
                            AddToIncomingMessageQueue();
                        } else {
                            std::cout << "[" << id << "] Read Body Fail.\n";
                            m_socket.close();
                        }
                    });
            }

            // Add a full message to the queue, once it arrives
            void AddToIncomingMessageQueue()
            {
                if (m_nOwnerType == owner::server) m_qMessagesIn.push_back({ this->shared_from_this(), m_msgTemporaryIn });
                else m_qMessagesIn.push_back({ nullptr, m_msgTemporaryIn });

                // Wait for next message
                ReadHeader();
            }

            // Encrypt data - will need to change later on because this is a form of security through obscurity
            uint64_t scramble(uint64_t nInput)
            {
                uint64_t out = nInput ^ 0xDEADBEEFC0DECAFE;
                out = (out & 0xF0F0F0F0F0F0F0) >> 6 | (out & 0x0F0F0F0F0F0F0F) << 2;
                return out ^ 0x12345678C0DEFACE;
            }

            // Async - Used by both the client and server to write validation packet
            void WriteValidation()
            {
                asio::async_write(m_socket, asio::buffer(&m_nHandshakeOut, sizeof(uint64_t)),
                    [this](std::error_code ec, std::size_t length)
                    {
                        // Validation data sent, client should wait
                        if (!ec) {
                            if (m_nOwnerType == owner::client) ReadHeader();
                        } else {
                            m_socket.close();
                        }
                    });
            }

            void ReadValidation(kim::net::server_interface<T> *server = nullptr)
            {
                asio::async_read(m_socket, asio::buffer(&m_nHandshakeIn, sizeof(uint64_t)),
                    [this, server](std::error_code ec, std::size_t length)
                    {
                        if (!ec) {
                            if (m_nOwnerType == owner::server) {
                                if (m_nHandshakeIn == m_nHandshakeCheck) {
                                    // Connect properly
                                    std::cout << "Client Validated" << std::endl;
                                    server->OnClientValidated(this->shared_from_this());

                                    ReadHeader();
                                } else {
                                    std::cout << "Client Disconnected (Failed Validation)" << std::endl;
                                    m_socket.close();
                                }
                            } else {
                                // Connection is a client, so solve the puzzle
                                m_nHandshakeOut = scramble(m_nHandshakeIn);

                                WriteValidation();
                            }
                        } else {
                            std::cout << "Client Disconnected (ReadValidation)" << std::endl;
                            m_socket.close();
                        }
                    });
            }

        protected:
            // Each connection has a unique socket to a remote
            asio::ip::tcp::socket m_socket;

            // This context is shared with the whole ASIO instance
            asio::io_context &m_asioContext;

            // This queue holds all messages to be sent to the 
            // remote side of this connection
            tsqueue<message<T>> m_qMessagesOut;

            // This queue holds all messages that have been received from
            // the remote side of this connection. Note it is a reference
            // as the "owner" of this connection is expected to provide a queue
            tsqueue<owned_message<T>> &m_qMessagesIn;

            // Incoming messages are temporarily stored and assembled here, asynchronously
            message<T> m_msgTemporaryIn;

            // The "owner" decides how some of the connection behaves
            owner m_nOwnerType = owner::server;
            uint32_t id = 0; 

            // Handshake validation
            uint64_t m_nHandshakeOut = 0;
            uint64_t m_nHandshakeIn = 0;
            uint64_t m_nHandshakeCheck = 0;
        };
    }
}