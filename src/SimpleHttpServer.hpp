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
            void Respond()
            {
                std::time_t now = std::time(0);
                std::string time = std::ctime(&now);
                std::ostringstream sstr;
                sstr << "HTTP/1.0 200 OK\r\n" <<
                "Content-Type: text/html; charset=UTF-8\r\n" <<
                "Content-Length: " << time.length() + 2 << "\r\n\r\n" <<
                time << "\r\n";

                Respond(sstr.str()); 
            }

            void Respond(const std::string& respondData)
            {
                auto self(shared_from_this());
                m_Socket.async_write_some(asio::buffer(respondData.data(), respondData.size()), 
                    [this, self] (const asio::error_code& ec, size_t bytesTransfered)
                    {
                        if(!ec)
                        {
                            m_Socket.shutdown(asio::ip::tcp::socket::shutdown_both);
                        }
                        else
                        {
                            m_Socket.close();
                        }
                    }
                );
            }

            void ReadBody()
            {
                auto self(shared_from_this());
                Respond();
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
                        ReadBody();
                        if (ec)
                            m_Socket.close();
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

        ~HttpServer()
        {
            if (m_ContextThread->joinable())
                m_ContextThread->join();
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
                    if (!m_Acceptor.is_open())
                        return;

                    if(!ec)
                        std::make_shared<Details::RequestSession>(std::move(socket))->Start();
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
