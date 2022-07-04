#include <memory>
#include <thread>
#include <sstream>
#include <iostream>

#include <asio.hpp>

namespace Simple {
    class HttpServer;

    namespace Details
    {
        class RequestSession: public std::enable_shared_from_this<RequestSession>
        {
        public:
            RequestSession(asio::ip::tcp::socket socket) :
                m_Socket(std::move(socket))
            {
            }

            void Start()
            {
                ReadHeader();
            }

        private:
            void ReadBody()
            {

            }

            void ReadHeader()
            {
                auto self(shared_from_this());
                asio::async_read_until(m_Socket, m_RequestBuffer, "\r\n\r\n",
                    [this, self] (const asio::error_code& ec, size_t bytesTransfered)
                    {
                        std::ostringstream ss;
                        ss << &m_RequestBuffer;
                        std::cout << "Request header:\n" << ss.str();
                        if (ec)
                        {
                            m_Socket.close();
                        }
                    }
                );
            }

        private:
            asio::ip::tcp::socket m_Socket;
            asio::streambuf m_RequestBuffer;
        };
    }

    class HttpServer
    {
    public:
        HttpServer(const std::string& address, uint_least16_t port) :
            m_Acceptor(m_IoContext, asio::ip::tcp::endpoint(asio::ip::address::from_string(address), port))
        {
        }

        void Start()
        {
            m_ContextThread = std::make_shared<std::thread>(
                [this] ()
                {
                    DoAccept();
                    m_IoContext.run();
                }
            );
        }

    private:
        void DoAccept()
        {
            m_Acceptor.async_accept(
                [this] (const asio::error_code& ec, asio::ip::tcp::socket socket)
                {
                    if(!ec)
                    {
                        std::make_shared<Details::RequestSession>(std::move(socket))->Start();
                    }
                    DoAccept();
                }
            );
        }

    private:
        asio::io_context m_IoContext;
        asio::ip::tcp::acceptor m_Acceptor;
        std::shared_ptr<std::thread> m_ContextThread;
    };
}
