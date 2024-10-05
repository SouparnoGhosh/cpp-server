#include "HTTPServer.h"
#include <iostream> /* For stream based input/output, imports cout, cin */
#include <cstring> /* For manipulating C-style strings, imports strlen() */
#include <unistd.h> /* For POSIX operating system API for file descriptors, imports close() */
#include <sys/socket.h> /* Imports AF_INET = 2, SOCK_STREAM = 1, socket(), bind(), listen(), accept() */
#include <netinet/in.h> /* Imports INADRR_ANY = 0 */
#include <arpa/inet.h> /* Imports htons() */
#include <cstdlib> /* Imports exit(), EXIT_FAILURE = 1 */


using namespace std;

/*  This is a constructor function. :: is the scope resolution operator.
    It is required when creating methods of the class outside its declaration.
    address structure is a private attribute of the class HTTPServer
    The worflow -> create a socket -> set up the address structure of the socket -> bind the address to the socket
*/
HTTPServer::HTTPServer(int server_port) : server_port(server_port), server_addrlen(sizeof(server_address)) {
    /* 
        socket() creates a new socket. It returns a file descriptor.
        AF_INET is the address family for IPv4.
        SOCK_STREAM is the type of the socket. It is a stream socket which is used for TCP.
        
        0 says to follow the default protocol for the specified address family and socket type.
        server_fd = socket() assigns the file descriptor of the socket to the server_fd variable.
        
        now the condition checks if server_fd is 0. If it is 0, it means the socket creation failed.
    */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* Set the socket option to reuse the address */
    /* 
        opt is used to set the socket option.
        setsockopt() sets the socket options.

        server_fd = file descriptor of the socket on which to set the options.
        level = SOL_SOCKET = level at which the option is defined. It is the socket layer here. Value = 1.
        option name = SO_REUSEADDR = option to set. It allows the socket to bind to an address that is already in use. Value = 2.
        &opt = pointer to the value of the option.
        sizeof(opt) = size of the option value.

        SO_REUSEADDR allows a socket to forcibly bind to a port in use by another socket in TIME_WAIT state.

        If setsockopt() returns 0, it means the setsockopt() function succeeded.
        If setsockopt() returns -1, it means the setsockopt() function failed.

        opt = 1 means to enable the option.
        opt = 0 means to disable the option.
    */
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /*  address is sockadder_in structure. 
        sin = socket internet.

        AF_INET is for IPv4 address family. Address Family Internet.
        sin_addr.s_addr is the IP address.
        
        INADDR_ANY is a constant that binds the socket to all available interfaces.
        INADDR_ANY is Internet Address Any.
        
        htons(port) converts the port from host byte order to network byte order.
        htons = host to network short. It converts the byte order of the port to big endian format.
        Network protocols use big endian format.
    */
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(server_port);

    /* 
        bind() binds the socket to the address and port number specified in the address structure.
        If the bind() function fails, it returns -1. If it succeeds, it returns 0.
        
        The arguments are - 
        file descriptor of the socket.
        pointer to the sockaddr structure, the address attribute of the class.
        size of the address structure.
    */
    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
}

/* This is the destructor function. close() can automatically detect file descriptors*/
HTTPServer::~HTTPServer() {
    close(server_fd);
}

void HTTPServer::start() {
    /* 
        Listen for incoming connections
        It puts the server socket in a passive mode, where it waits for the client to approach the server to make a connection.
        3 is the backlog parameter. It defines the maximum length to which the queue of pending connections for server_fd may grow.
        
        If listen() returns 0, it means the listen() function succeeded.
        If listen() returns -1, it means the listen() function failed. 
        So close the file descriptor and exit the program.
    */
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    cout << "Server is listening on address " << inet_ntoa(server_address.sin_addr) << ":" << ntohs(server_address.sin_port) << endl;
    cout << "Server file descriptor is " << server_fd << endl;

    /* 
        The server will continously accept and handle incoming connections from the client.    
    */
    while (true) {
        /* Create a file descriptor for the client socket */
        int client_fd;
        struct sockaddr_in client_address;
        socklen_t client_addrlen = sizeof(client_address);
        /*
            accept() is called to accept an incoming connection request.
            server_fd is the file descriptor of the server socket listening for connections.
            address is pointer to address of client. The OS provides the address of the client when the server accepts the connection.
            addrlen is the size of the address structure.
            this is assigned to client_fd.

            If client_fd is positive, it means the accept() function succeeded.
            If client_fd is -1, it means the accept() function failed.
        */
        if ((client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t*)&client_addrlen)) < 0) {
            perror("accept failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        /* 
            inet_ntoa() converts the IP address from binary to string dotted-decimal form.
            ntohs() converts the port number from network byte order to host byte order.
            This port will always be different in each call because the OS will assign a new ephemeral port for each TCP call.
        */
        cout << "Accepted connection from " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << endl;
        handleClient(client_fd);
    } 
}

/* 
    Handles the client connection once the client is accepted.
    Reads the client request, sends a response, and closes the client connection.
*/
void HTTPServer::handleClient(int client_fd) {
    /* 
        Create a buffer and initialize all elements to 0.
        It is used to store the data read from the client.
    */
    char buffer[1024] = {0};
    /*
        character pointer hello -> an HTTP response message. 
        
        The response includes:
            HTTP/1.1 200 OK: HTTP status line indicating a successful request.
            Content-Type: text/plain: Header specifying the content type as plain text.
            Content-Length: 12: Header specifying the length of the response body.
            Hello world!: The response body.
    */
    const char *hello = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";

    /* 
        read() reads the data from the client and stores it in the buffer.
        
        The data is read from the client_fd file descriptor.
        It is stored in the buffer.
        Max no of bytes to read from client_fd = 1024.
        
        The read() function returns the number of bytes read.
    */
    int  bytes_read = read(client_fd, buffer, 1024);

    cout << "Bytes read from client -> " << bytes_read << endl;
    cout << "\nReceived request:\n" << buffer << endl;

    /* 
        Send() sends data over the client socket.
        hello - pointer to the response message.
        flag parameter = 0. It means no special options are set for the send operation. 
    */
    int bytes_send = send(client_fd, hello, strlen(hello), 0);

    cout << "Bytes sent to client -> " << bytes_send << endl;
    cout << "Hello message sent" << endl;

    cout << "Client file descriptor -> " << client_fd << endl;
    close(client_fd);
}