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
    server.Get("/", [] (const Simple::Request& req, Simple::Response& res) {
      for (auto& [name, value] : req.headers)
        std::cout << name << ": " << value << '\n';
      for (auto& [name, value] : req.headers)
        std::cout << name << ": " << value << '\n';
      res.body = "Saludos desde el servidor";
    });
    server.Put("/", [] (const Simple::Request& req, Simple::Response& res) {
      res.body = "Saludos desde el servidor";
    });
    server.Delete("/", [] (const Simple::Request& req, Simple::Response& res) {
      res.body = "Saludos desde el servidor";
    });
    server.Start();
  } catch(std::exception ex)
  {
    std::cout << ex.what();
  }
}
