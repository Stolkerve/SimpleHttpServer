#include "SimpleHttpServer.hpp"

int main()
{
  try
  {
    Simple::HttpServer server("127.0.0.1", 3000);
    server.Start();
  } catch(std::exception ex)
  {
    std::cout << ex.what();
  }
}
