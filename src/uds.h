#pragma once
#include <string>
#include <sys/socket.h>
#include <sys/un.h>

class UdsServerSocket
{
public:
    UdsServerSocket(const std::string &my_path, const std::string &target_path);
    ~UdsServerSocket();

    bool bindSocket();
    ssize_t send(const void *buffer, size_t length);
    ssize_t receive(void *buffer, size_t length);
    int getFd() const;
    const std::string &getMyPath() const;

private:
    int fd_;
    std::string my_path_;
    std::string target_path_;
    sockaddr_un server_addr_;
    sockaddr_un target_addr_;
};
