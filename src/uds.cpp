#include "uds.h"
#include <unistd.h>
#include <cstring>
#include <stdexcept>

UdsSocket::UdsSocket(const std::string &my_path, const std::string &target_path)
    : fd_(-1), my_path_(my_path), target_path_(target_path)
{
    memset(&server_addr_, 0, sizeof(server_addr_));
    server_addr_.sun_family = AF_UNIX;
    strncpy(server_addr_.sun_path, my_path_.c_str(), sizeof(server_addr_.sun_path) - 1);

    memset(&target_addr_, 0, sizeof(target_addr_));
    target_addr_.sun_family = AF_UNIX;
    strncpy(target_addr_.sun_path, target_path_.c_str(), sizeof(target_addr_.sun_path) - 1);

    fd_ = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd_ < 0)
    {
        throw std::runtime_error("Error creating UDS socket");
    }
}

UdsSocket::~UdsSocket()
{
    if (fd_ != -1)
    {
        close(fd_);
        if (!my_path_.empty())
        {
            unlink(my_path_.c_str());
        }
    }
}

bool UdsSocket::bindSocket()
{
    unlink(my_path_.c_str());
    if (bind(fd_, (struct sockaddr *)&server_addr_, sizeof(server_addr_)) < 0)
    {
        return false;
    }
    return true;
}

ssize_t UdsSocket::send(const void *buffer, size_t length)
{
    ssize_t sent = sendto(fd_, buffer, length, 0, (struct sockaddr *)&target_addr_, sizeof(target_addr_));
    if (sent < 0)
    {
        perror("[ERROR] UDS sendto failed");
    }
    return sent;
}

ssize_t UdsSocket::receive(void *buffer, size_t length)
{
    ssize_t received = recvfrom(fd_, buffer, length, 0, NULL, NULL);
    if (received < 0)
    {
        perror("[ERROR] UDS recvfrom failed");
    }
    return received;
}

int UdsSocket::getFd() const
{
    return fd_;
}

const std::string &UdsSocket::getMyPath() const
{
    return my_path_;
}
