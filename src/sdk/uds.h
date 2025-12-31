// uds.h - Unix Domain Socket wrapper for FSL
//
// UdsSocket provides a simple interface for creating, binding, sending, and receiving
// datagrams over Unix Domain Sockets (UDS). Used for both server (downlink) and client (uplink)
// communication between FSL and application processes.
//
// Usage:
//   - Server: UdsSocket(server_path, "")
//   - Client: UdsSocket("", target_path)
//
// Methods:
//   - bindSocket(): Bind the socket to my_path_ (for servers)
//   - send(): Send a datagram to target_path_
//   - receive(): Receive a datagram from the socket
//   - getFd(): Get the socket file descriptor
//   - getMyPath(): Get the bound path (server)

#pragma once
#include <string>
#include <sys/socket.h>
#include <sys/un.h>

// UdsSocket: Unix Domain Socket wrapper for FSL
class UdsSocket
{
public:
    // Constructor: create UDS socket for server (my_path) or client (target_path)
    UdsSocket(const std::string &my_path, const std::string &target_path);

    // Set SO_RCVBUF for this socket (call before bindSocket)
    bool setReceiveBufferSize(int size);

    // Destructor: closes socket and unlinks my_path
    ~UdsSocket();

    // Bind the socket to my_path (server)
    bool bindSocket();

    // Send a datagram to target_path (client)
    virtual ssize_t send(const void *buffer, size_t length);

    // Receive a datagram from the socket
    ssize_t receive(void *buffer, size_t length);

    // Get the socket file descriptor
    int getFd() const;

    // Get the bound path (server)
    const std::string &getMyPath() const;

private:
    int fd_;
    std::string my_path_;
    std::string target_path_;
    sockaddr_un server_addr_;
    sockaddr_un target_addr_;
};
