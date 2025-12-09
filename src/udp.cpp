#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include "udp.h"

UdpServerSocket::UdpServerSocket(int local_port, const std::string &remote_ip, int remote_port)
    : fd_(-1), local_port_(local_port), remote_ip_(remote_ip), remote_port_(remote_port)
{
    memset(&local_addr_, 0, sizeof(local_addr_));
    local_addr_.sin_family = AF_INET;
    local_addr_.sin_port = htons(local_port_);
    local_addr_.sin_addr.s_addr = INADDR_ANY;

    memset(&remote_addr_, 0, sizeof(remote_addr_));
    remote_addr_.sin_family = AF_INET;
    remote_addr_.sin_port = htons(remote_port_);
    if (inet_pton(AF_INET, remote_ip_.c_str(), &remote_addr_.sin_addr) <= 0)
    {
        throw std::runtime_error("Invalid Remote IP Address: " + remote_ip_);
    }

    fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0)
    {
        throw std::runtime_error("Error creating UDP socket");
    }

    int reuse = 1;
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        close(fd_);
        throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
    }
}

UdpServerSocket::~UdpServerSocket()
{
    if (fd_ != -1)
    {
        close(fd_);
    }
}

bool UdpServerSocket::bindSocket()
{
    if (bind(fd_, (struct sockaddr *)&local_addr_, sizeof(local_addr_)) < 0)
    {
        return false;
    }
    return true;
}

ssize_t UdpServerSocket::send(const void *buffer, size_t length)
{
    ssize_t sent = sendto(fd_, buffer, length, 0, (struct sockaddr *)&remote_addr_, sizeof(remote_addr_));
    if (sent < 0)
    {
        perror("[ERROR] UDP sendto failed");
    }
    return sent;
}

ssize_t UdpServerSocket::receive(void *buffer, size_t length, sockaddr_in *sender_addr)
{
    socklen_t sender_len = sizeof(sockaddr_in);
    ssize_t received;
    if (sender_addr)
    {
        received = recvfrom(fd_, buffer, length, 0, (struct sockaddr *)sender_addr, &sender_len);
    }
    else
    {
        received = recvfrom(fd_, buffer, length, 0, NULL, NULL);
    }
    if (received < 0)
    {
        perror("[ERROR] UDP recvfrom failed");
    }
    return received;
}

int UdpServerSocket::getFd() const
{
    return fd_;
}
