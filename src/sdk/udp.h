// udp.h - UDP Socket wrapper
//
// UdpServerSocket provides a simple interface for creating, binding, sending, and receiving
// datagrams over UDP sockets.
//
// Usage:
//   - UdpServerSocket(local_port, remote_ip, remote_port)
//   - bindSocket(): Bind the socket to local_port
//   - send(): Send a datagram to remote_ip:remote_port
//   - receive(): Receive a datagram from the socket
//   - getFd(): Get the socket file descriptor
//
// Error handling: Throws std::runtime_error on socket creation/binding errors.

#pragma once
#include <string>
#include <netinet/in.h>

// UdpServerSocket: UDP socket wrapper
class UdpServerSocket
{
public:
    // Constructor: create UDP socket for given local port and remote IP/port
    UdpServerSocket(int local_port, const std::string &remote_ip, int remote_port);

    // Destructor: closes socket
    ~UdpServerSocket();

    // Bind the socket to local_port
    bool bindSocket();

    // Send a datagram to remote_ip:remote_port
    ssize_t send(const void *buffer, size_t length);

    // Receive a datagram from the socket
    ssize_t receive(void *buffer, size_t length, sockaddr_in *sender_addr = nullptr);

    // Get the socket file descriptor
    int getFd() const;

private:
    int fd_;
    int local_port_;
    std::string remote_ip_;
    int remote_port_;
    sockaddr_in local_addr_;
    sockaddr_in remote_addr_;
};
