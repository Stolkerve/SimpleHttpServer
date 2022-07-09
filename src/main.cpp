#include "SimpleHttpServer.hpp"

int main()
{
  try
  {
    Simple::HttpServer server("127.0.0.1", 3000);
    server.Post("/", [] (const Simple::Request& req, Simple::Response& res) {
        std::cout << req.contentLength << "\n";

        res.body = "Saludos desde el servidor\n";
      }
    );
    server.Start();
  } catch(std::exception ex)
  {
    std::cout << ex.what();
  }
}
