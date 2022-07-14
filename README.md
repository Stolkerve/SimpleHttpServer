SimpleHttpServer
================
##  SimpleHttpServer is a simple and minimal http asynchronous server like expressJS

Generate project
================
The premake5 project generator is needed. To download https://premake.github.io/
\
***premake5 > cmake***
## Windows:
```
premake5 vs2019
```

## Linux:
```
premake5 gmake2
```

Compile
================
## Linux
#### Debug
```
make
```
#### Release
```
make config=release
```
## Windows:
#### Hit compile buttom of Visual Studio

Example
========
``` cpp

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

```
