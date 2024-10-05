#include"HTTPServer.h"

int main() {
    HTTPServer server(8080);
    server.start();
    return 0;
}