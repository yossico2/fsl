#pragma once
#include <string>
#include <netinet/in.h>

class UdpServerSocket {
public:
    UdpServerSocket(int local_port, const std::string& remote_ip, int remote_port);
    ~UdpServerSocket();

    bool bindSocket();
    ssize_t send(const void* buffer, size_t length);
    ssize_t receive(void* buffer, size_t length, sockaddr_in* sender_addr = nullptr);
    int getFd() const;

private:
    int fd_;
    int local_port_;
    std::string remote_ip_;
    int remote_port_;
    sockaddr_in local_addr_;
    sockaddr_in remote_addr_;
};
