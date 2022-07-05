#pragma once

#include <memory>
#include <thread>
#include <sstream>
#include <iostream>
#include <vector>

#include <asio.hpp>

namespace Simple {
    struct Header
    {
        std::string name;
        std::string value;
    };

    struct Params
    {
        std::string name;
        std::string value;
    };

    struct Request
    {
        std::string method;
        std::string path; 
        std::string body; 
        std::string remote_addr;
        std::string version;
        std::string target;
        int versionMajor;
        int versionMinor;
        uint32_t contentLength;
        std::vector<Header> headers;
        std::vector<Params> params;
    };

    namespace Details
    {
        constexpr auto END_TOKEN = "\r\n\r\n"; 
        // Check if a byte is an HTTP character.
        inline bool IsChar(int c)
        {
            return c >= 0 && c <= 127;
        }

        // Check if a byte is an HTTP control character.
        inline bool IsControl(int c)
        {
            return (c >= 0 && c <= 31) || (c == 127);
        }

        // Check if a byte is defined as an HTTP special character.
        inline bool IsSpecial(int c)
        {
            switch (c)
            {
            case '(': case ')': case '<': case '>': case '@':
            case ',': case ';': case ':': case '\\': case '"':
            case '/': case '[': case ']': case '?': case '=':
            case '{': case '}': case ' ': case '\t':
                return true;
            default:
                return false;
            }
        }

        // Check if a byte is a digit.
        inline bool IsDigit(int c)
        {
            return c >= '0' && c <= '9';
        }

        inline bool CheckIfConnection(const Header &item)
        {
            return strcasecmp(item.name.c_str(), "Connection") == 0;
        }

        // The current state of the parser.
        enum State
        {
            RequestMethodStart,
            RequestMethod,
            RequestUriStart,
            RequestUri,
            RequestHttpVersion_h,
            RequestHttpVersion_ht,
            RequestHttpVersion_htt,
            RequestHttpVersion_http,
            RequestHttpVersion_slash,
            RequestHttpVersion_majorStart,
            RequestHttpVersion_major,
            RequestHttpVersion_minorStart,
            RequestHttpVersion_minor,

            ResponseStatusStart,
            ResponseHttpVersion_ht,
            ResponseHttpVersion_htt,
            ResponseHttpVersion_http,
            ResponseHttpVersion_slash,
            ResponseHttpVersion_majorStart,
            ResponseHttpVersion_major,
            ResponseHttpVersion_minorStart,
            ResponseHttpVersion_minor,
            ResponseHttpVersion_spaceAfterVersion,
            ResponseHttpVersion_statusCodeStart,
            ResponseHttpVersion_spaceAfterStatusCode,
            ResponseHttpVersion_statusTextStart,
            ResponseHttpVersion_newLine,

            HeaderLineStart,
            HeaderLws,
            HeaderName,
            SpaceBeforeHeaderValue,
            HeaderValue,
            ExpectingNewline_2,
            ExpectingNewline_3,

            Post,
        };

        enum ParseResult
        {
            ParsingCompleted,
            ParsingIncompleted,
            ParsingError
        };

        ParseResult ParseRequest(const std::string& requestData, Request& req)
        {
            // Parser from https://github.com/nekipelov/httpparser

            State state = RequestMethodStart;
            bool chunked = false;
            size_t contentSize = 0;
            
            for (char input : requestData)
            {
                switch (state)
                {
                case RequestMethodStart:
                    if( !IsChar(input) || IsControl(input) || IsSpecial(input) )
                    {
                        return ParsingError;
                    }
                    else
                    {
                        state = RequestMethod;
                        req.method.push_back(input);
                    }
                    break;
                case RequestMethod:
                    if( input == ' ' )
                    {
                        state = RequestUriStart;
                    }
                    else if( !IsChar(input) || IsControl(input) || IsSpecial(input) )
                    {
                        return ParsingError;
                    }
                    else
                    {
                        req.method.push_back(input);
                    }
                    break;
                case RequestUriStart:
                    if( IsControl(input) )
                    {
                        return ParsingError;
                    }
                    else
                    {
                        state = RequestUri;
                        req.path.push_back(input);
                    }
                    break;
                case RequestUri:
                    if( input == ' ' )
                    {
                        state = RequestHttpVersion_h;
                    }
                    else if (input == '\r')
                    {
                        req.versionMajor = 0;
                        req.versionMinor = 9;

                        return ParsingCompleted;
                    }
                    else if( IsControl(input) )
                    {
                        return ParsingError;
                    }
                    else
                    {
                        req.path.push_back(input);
                    }
                    break;
                case RequestHttpVersion_h:
                    if( input == 'H' )
                    {
                        state = RequestHttpVersion_ht;
                    }
                    else
                    {
                        return ParsingError;
                    }
                    break;
                case RequestHttpVersion_ht:
                    if( input == 'T' )
                    {
                        state = RequestHttpVersion_htt;
                    }
                    else
                    {
                        return ParsingError;
                    }
                    break;
                case RequestHttpVersion_htt:
                    if( input == 'T' )
                    {
                        state = RequestHttpVersion_http;
                    }
                    else
                    {
                        return ParsingError;
                    }
                    break;
                case RequestHttpVersion_http:
                    if( input == 'P' )
                    {
                        state = RequestHttpVersion_slash;
                    }
                    else
                    {
                        return ParsingError;
                    }
                    break;
                case RequestHttpVersion_slash:
                    if( input == '/' )
                    {
                        req.versionMajor = 0;
                        req.versionMinor = 0;
                        state = RequestHttpVersion_majorStart;
                    }
                    else
                    {
                        return ParsingError;
                    }
                    break;
                case RequestHttpVersion_majorStart:
                    if( IsDigit(input) )
                    {
                        req.versionMajor = input - '0';
                        state = RequestHttpVersion_major;
                    }
                    else
                    {
                        return ParsingError;
                    }
                    break;
                case RequestHttpVersion_major:
                    if( input == '.' )
                    {
                        state = RequestHttpVersion_minorStart;
                    }
                    else if (IsDigit(input))
                    {
                        req.versionMajor = req.versionMajor * 10 + input - '0';
                    }
                    else
                    {
                        return ParsingError;
                    }
                    break;
                case RequestHttpVersion_minorStart:
                    if( IsDigit(input) )
                    {
                        req.versionMinor = input - '0';
                        state = RequestHttpVersion_minor;
                    }
                    else
                    {
                        return ParsingError;
                    }
                    break;
                case RequestHttpVersion_minor:
                    if( input == '\r' )
                    {
                        state = ResponseHttpVersion_newLine;
                    }
                    else if( IsDigit(input) )
                    {
                        req.versionMinor = req.versionMinor * 10 + input - '0';
                    }
                    else
                    {
                        return ParsingError;
                    }
                    break;
                case ResponseHttpVersion_newLine:
                    if( input == '\n' )
                    {
                        state = HeaderLineStart;
                    }
                    else
                    {
                        return ParsingError;
                    }
                    break;
                case HeaderLineStart:
                    if( input == '\r' )
                    {
                        state = ExpectingNewline_3;
                    }
                    else if( !req.headers.empty() && (input == ' ' || input == '\t') )
                    {
                        state = HeaderLws;
                    }
                    else if( !IsChar(input) || IsControl(input) || IsSpecial(input) )
                    {
                        return ParsingError;
                    }
                    else
                    {
                        req.headers.push_back(Header());
                        req.headers.back().name.reserve(16);
                        req.headers.back().value.reserve(16);
                        req.headers.back().name.push_back(input);
                        state = HeaderName;
                    }
                    break;
                case HeaderLws:
                    if( input == '\r' )
                    {
                        state = ExpectingNewline_2;
                    }
                    else if( input == ' ' || input == '\t' )
                    {
                    }
                    else if( IsControl(input) )
                    {
                        return ParsingError;
                    }
                    else
                    {
                        state = HeaderValue;
                        req.headers.back().value.push_back(input);
                    }
                    break;
                case HeaderName:
                    if( input == ':' )
                    {
                        state = SpaceBeforeHeaderValue;
                    }
                    else if( !IsChar(input) || IsControl(input) || IsSpecial(input) )
                    {
                        return ParsingError;
                    }
                    else
                    {
                        req.headers.back().name.push_back(input);
                    }
                    break;
                case SpaceBeforeHeaderValue:
                    if( input == ' ' )
                    {
                        state = HeaderValue;
                    }
                    else
                    {
                        return ParsingError;
                    }
                    break;
                case HeaderValue:
                    if( input == '\r' )
                    {
                        if( req.method == "POST" || req.method == "PUT" )
                        {
                            Header &h = req.headers.back();

                            if( strcasecmp(h.name.c_str(), "Content-Length") == 0 )
                            {
                                contentSize = atoi(h.value.c_str());
                                req.body.reserve( contentSize );
                                req.contentLength = contentSize;
                            }
                            else if( strcasecmp(h.name.c_str(), "Transfer-Encoding") == 0 )
                            {
                                if(strcasecmp(h.value.c_str(), "chunked") == 0)
                                    chunked = true;
                            }
                        }
                        state = ExpectingNewline_2;
                    }
                    else if( IsControl(input) )
                    {
                        return ParsingError;
                    }
                    else
                    {
                        req.headers.back().value.push_back(input);
                    }
                    break;
                case ExpectingNewline_2:
                    if( input == '\n' )
                    {
                        state = HeaderLineStart;
                    }
                    else
                    {
                        return ParsingError;
                    }
                    break;
                case ExpectingNewline_3: {
                    std::vector<Header>::iterator it = std::find_if(req.headers.begin(),
                                                                        req.headers.end(),
                                                                        CheckIfConnection);

                    if( it != req.headers.end() )
                    {
                        if( strcasecmp(it->value.c_str(), "Keep-Alive") == 0 )
                        {
                            // req.keepAlive = true;
                        }
                        else  // == Close
                        {
                            // req.keepAlive = false;
                        }
                    }
                    else
                    {
                        // if( req.versionMajor > 1 || (req.versionMajor == 1 && req.versionMinor == 1) )
                        //     req.keepAlive = true;
                    }

                    if( chunked )
                    {
                    }
                    else if( contentSize == 0 )
                    {
                        if( input == '\n')
                            return ParsingCompleted;
                        else
                            return ParsingError;
                    }
                    else
                    {
                        state = Post;
                    }
                    break;
                }
                case Post:
                    return ParsingCompleted;
                    break;
                default:
                    return ParsingError;
                }
            }

            return ParsingIncompleted;
        }

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

            void ReadBody(Request req)
            {
                auto self(shared_from_this());
                if (req.body.size() == req.contentLength)
                    Respond();

                bodyBuffer.clear(); 
                bodyBuffer.resize(m_Socket.available());
                m_Socket.async_read_some(asio::buffer(bodyBuffer, bodyBuffer.size()),
                    [this, self, req = std::move(req)] (const asio::error_code& ec, size_t bytesTransfered) mutable
                    {
                        if (!ec)
                        {
                            req.body.append(reinterpret_cast<char const*>(bodyBuffer.data()), bytesTransfered);
                            std::cout << req.body;
                            Respond();
                        }
                        else if (ec == asio::error::operation_aborted)
                            m_Socket.close();
                    }
                );
            }

            void ReadHeader()
            {
                auto self(shared_from_this());
                asio::async_read_until(m_Socket, m_RequestBuffer, END_TOKEN,
                    [this, self] (const asio::error_code& ec, size_t bytesTransfered)
                    {
                        std::ostringstream ss;
                        ss << &m_RequestBuffer;

                        const std::string& requestStr = ss.str();

                        Request req; 
                        Details::ParseRequest(ss.str(), req);

                        auto found = requestStr.find(END_TOKEN) + std::strlen(END_TOKEN);
                        size_t bytesExceeded = requestStr.size() - found;
                        req.body.append(requestStr.substr(found));

                        ReadBody(std::move(req));

                        if (ec)
                            m_Socket.close();
                    }
                );
            }

        private:
            asio::ip::tcp::socket m_Socket;
            asio::streambuf m_RequestBuffer;
            std::vector<uint8_t> bodyBuffer;
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
