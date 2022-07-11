#pragma once

#include <unordered_map>
#include <functional>
#include <memory>
#include <thread>
#include <sstream>
#include <iostream>
#include <vector>
#include <iomanip>

#include <asio.hpp>

namespace Simple {
    namespace Details { class RequestSession; }
    typedef std::unordered_map<std::string, std::string> Headers;
    typedef std::unordered_map<std::string, std::string> Params;
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
        Headers headers;
        Params params;
    };

    struct Response
    {
        Response()
        {
            headers["Content-Type"] = "text/plain";
            headers["Content-Length"] = "0";
        };

        void SetContentType(const std::string& value)
        {
            headers["Content-Type"] = value;
        }

        void SetAccessControlAllowOrigin(const std::string& value)
        {
            headers["Access-Control-Allow-Origin"] = value;
        }

        uint16_t status = 200;
        std::string version;
        std::string body;
        std::string location; // Redirect location>
        Headers headers;
    };

    typedef std::function<void(const Request&, Response&)> CallbackHandler;
    typedef std::pair<std::string, CallbackHandler> Handler;

    class HttpServer
    {
    public:
        HttpServer(const std::string& address, uint_least16_t port);
        ~HttpServer();
        void Start();
        void Get(const std::string& pathPattern, CallbackHandler requestHandler);
        void Post(const std::string& pathPattern, CallbackHandler requestHandler);
        void Put(const std::string& pathPattern, CallbackHandler requestHandler);
        void Delete(const std::string& pathPattern, CallbackHandler requestHandler);


    private:
        void DoAccept();

    private:
        asio::io_context m_IoContext;
        asio::ip::tcp::acceptor m_Acceptor;

        std::vector<Handler> m_GetHandlers;
        std::vector<Handler> m_PostHandlers;
        std::vector<Handler> m_PutHandlers;
        std::vector<Handler> m_DeleteHandlers;
        std::shared_ptr<std::thread> m_ContextThread;

        friend class Details::RequestSession;
    };

    namespace Details
    {
        inline const char* StatusMessage(uint16_t status) {
            switch (status) {
            case 100: return "Continue";
            case 101: return "Switching Protocol";
            case 102: return "Processing";
            case 103: return "Early Hints";
            case 200: return "OK";
            case 201: return "Created";
            case 202: return "Accepted";
            case 203: return "Non-Authoritative Information";
            case 204: return "No Content";
            case 205: return "Reset Content";
            case 206: return "Partial Content";
            case 207: return "Multi-Status";
            case 208: return "Already Reported";
            case 226: return "IM Used";
            case 300: return "Multiple Choice";
            case 301: return "Moved Permanently";
            case 302: return "Found";
            case 303: return "See Other";
            case 304: return "Not Modified";
            case 305: return "Use Proxy";
            case 306: return "unused";
            case 307: return "Temporary Redirect";
            case 308: return "Permanent Redirect";
            case 400: return "Bad Request";
            case 401: return "Unauthorized";
            case 402: return "Payment Required";
            case 403: return "Forbidden";
            case 404: return "Not Found";
            case 405: return "Method Not Allowed";
            case 406: return "Not Acceptable";
            case 407: return "Proxy Authentication Required";
            case 408: return "Request Timeout";
            case 409: return "Conflict";
            case 410: return "Gone";
            case 411: return "Length Required";
            case 412: return "Precondition Failed";
            case 413: return "Payload Too Large";
            case 414: return "URI Too Long";
            case 415: return "Unsupported Media Type";
            case 416: return "Range Not Satisfiable";
            case 417: return "Expectation Failed";
            case 418: return "I'm a teapot";
            case 421: return "Misdirected Request";
            case 422: return "Unprocessable Entity";
            case 423: return "Locked";
            case 424: return "Failed Dependency";
            case 425: return "Too Early";
            case 426: return "Upgrade Required";
            case 428: return "Precondition Required";
            case 429: return "Too Many Requests";
            case 431: return "Request Header Fields Too Large";
            case 451: return "Unavailable For Legal Reasons";
            case 501: return "Not Implemented";
            case 502: return "Bad Gateway";
            case 503: return "Service Unavailable";
            case 504: return "Gateway Timeout";
            case 505: return "HTTP Version Not Supported";
            case 506: return "Variant Also Negotiates";
            case 507: return "Insufficient Storage";
            case 508: return "Loop Detected";
            case 510: return "Not Extended";
            case 511: return "Network Authentication Required";

            default:
            case 500: return "Internal Server Error";
            }
        }

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

        inline bool CheckIfConnection(const std::pair<std::string, std::string>& item)
        {
            return strcasecmp(item.first.c_str(), "Connection") == 0;
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

        inline ParseResult ParseRequest(const std::string& requestData, Request& req)
        {
            // Parser from https://github.com/nekipelov/httpparser
            std::vector<std::pair<std::string, std::string>> headers;
            std::vector<std::pair<std::string, std::string>> params;
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
                        headers.push_back({});
                        headers.back().first.reserve(16);
                        headers.back().second.reserve(16);
                        headers.back().first.push_back(input);
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
                        headers.back().second.push_back(input);
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
                        headers.back().first.push_back(input);
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
                            auto &h = headers.back();

                            if( strcasecmp(h.first.c_str(), "Content-Length") == 0 )
                            {
                                contentSize = atoi(h.second.c_str());
                                req.body.reserve( contentSize );
                                req.contentLength = contentSize;
                            }
                            else if( strcasecmp(h.first.c_str(), "Transfer-Encoding") == 0 )
                            {
                                if(strcasecmp(h.second.c_str(), "chunked") == 0)
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
                        headers.back().second.push_back(input);
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
                    std::vector<std::pair<std::string, std::string>>::iterator it = std::find_if(headers.begin(),
                                                                        headers.end(),
                                                                        CheckIfConnection);

                    if( it != headers.end() )
                    {
                        if( strcasecmp(it->second.c_str(), "Keep-Alive") == 0 )
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
                        {
                            for (auto& [name, value] : headers)
                                req.headers[name] = std::move(value);
                            for (auto& [name, value] : params)
                                req.params[name] = std::move(value);
                            return ParsingCompleted;
                        }
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
                    if (true)
                    {
                        for (auto& [name, value] : headers)
                            req.headers[name] = std::move(value);
                        for (auto& [name, value] : params)
                            req.params[name] = std::move(value);
                    }
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
            RequestSession(asio::ip::tcp::socket socket, HttpServer* server) :
                m_Socket(std::move(socket)), m_Server(server)
            {
            }
            void Start()
            {
                ReadHeader();
            }

        private:
            void MatchRequest(const std::vector<Handler>& handlers, const Request& req, Response& respond, bool& anyMatch)
            {
                for (auto& handler : handlers)
                    if (handler.first == req.path)
                    {
                        handler.second(req, respond);
                        anyMatch = true;
                    }
            }

            void Respond(Request req)
            {
                Response respond;
                
                bool anyMatch = false;
                if (req.method == "GET")
                    MatchRequest(m_Server->m_GetHandlers, req, respond, anyMatch);
                else if (req.method == "POST")
                    MatchRequest(m_Server->m_PostHandlers, req, respond, anyMatch);
                else if (req.method == "PUT")
                    MatchRequest(m_Server->m_PutHandlers, req, respond, anyMatch);
                else if (req.method == "DELETE")
                    MatchRequest(m_Server->m_DeleteHandlers, req, respond, anyMatch);
                
                if (!anyMatch)
                {
                    m_Socket.shutdown(asio::ip::tcp::socket::shutdown_both);
                    return;
                }

                respond.headers["Content-Type"] += "; charset=UTF-8";
                respond.headers["Content-Length"] = std::to_string(respond.body.size());    
                respond.headers["Connection"] = "close";
                respond.headers["Server"] = "SimpleHttpServer";
                if (!respond.location.empty()) respond.headers["Location"] = respond.location;
                std::time_t now = std::time(0);
                auto& date = respond.headers["Date"];
                date.resize(30);
                std::strftime((char *)date.c_str(), date.size(), "%a, %d %b %Y %T GMT", std::gmtime(&now));

                std::ostringstream finalResponse;
                finalResponse << "HTTP/1.1 " << respond.status << " " << Details::StatusMessage(respond.status) << "\r\n";
                for (auto& [name, value] : respond.headers)
                {
                    finalResponse << name + ": " << value << "\r\n";
                }
                finalResponse << "\r\n" <<
                respond.body;

                Respond(finalResponse.str()); 
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
                    Respond(std::move(req));

                bodyBuffer.clear(); 
                bodyBuffer.resize(m_Socket.available());
                m_Socket.async_read_some(asio::buffer(bodyBuffer, bodyBuffer.size()),
                    [this, self, req = std::move(req)] (const asio::error_code& ec, size_t bytesTransfered) mutable
                    {
                        if (!ec)
                        {
                            req.body.append(reinterpret_cast<char const*>(bodyBuffer.data()), bytesTransfered);

                            Respond(std::move(req));
                        }
                        else if (ec == asio::error::operation_aborted)
                            m_Socket.close();
                    }
                );
            }

            void ReadHeader()
            {
                auto self(shared_from_this());
                asio::async_read_until(m_Socket, m_RequestBuffer, Details::END_TOKEN,
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
            HttpServer* m_Server;
            asio::ip::tcp::socket m_Socket;
            asio::streambuf m_RequestBuffer;
            std::vector<uint8_t> bodyBuffer;
        };
    }

    HttpServer::HttpServer(const std::string& address, uint_least16_t port) :
        m_Acceptor(m_IoContext, asio::ip::tcp::endpoint(asio::ip::address::from_string(address), port))
    {
    }

    HttpServer::~HttpServer()
    {
        if (m_ContextThread->joinable())
            m_ContextThread->join();
    }
    void HttpServer::Start()
    {
        m_ContextThread = std::make_shared<std::thread>(
            [this] ()
            {
                DoAccept();
                m_IoContext.run();
            }
        );
    }
    void HttpServer::Get(const std::string& pathPattern, CallbackHandler requestHandler)
    {
        m_GetHandlers.push_back({pathPattern, requestHandler});
    }

    void HttpServer::Post(const std::string& pathPattern, CallbackHandler requestHandler)
    {
        m_PostHandlers.push_back({pathPattern, requestHandler});
    }

    void HttpServer::Put(const std::string& pathPattern, CallbackHandler requestHandler)
    {
        m_PutHandlers.push_back({pathPattern, requestHandler});
    }

    void HttpServer::Delete(const std::string& pathPattern, CallbackHandler requestHandler)
    {
        m_DeleteHandlers.push_back({pathPattern, requestHandler});
    }

    void HttpServer::DoAccept()
    {
        m_Acceptor.async_accept(
            [this] (const asio::error_code& ec, asio::ip::tcp::socket socket)
            {
                if (!m_Acceptor.is_open())
                    return;

                if(!ec)
                    std::make_shared<Details::RequestSession>(std::move(socket), this)->Start();

                DoAccept();
            }
        );
    }
}
