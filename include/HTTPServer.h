#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <netinet/in.h>

class HTTPServer {
    public:
        HTTPServer(int server_port);
        ~HTTPServer();
        void start();

    private:
        int server_port;
        int server_fd;
        struct sockaddr_in server_address;
        socklen_t server_addrlen;
        void handleClient(int client_fd);
};

#endif