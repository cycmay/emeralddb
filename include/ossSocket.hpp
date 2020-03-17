#ifndef _OSSSOCKET_HPP__
#define _OSSSOCKET_HPP__

#include "core.hpp"
#define SOCKET_GETLASTERROR errno
//by default 10ms timeout
#define OSS_SOCKET_DFT_TIMEOUT 10000

// max hostname
#define OSS_MAX_HOSTNAME NI_MAXHOST
#define OSS_MAX_SERVICENAME NI_MAXSERV

class _ossSocket
{
    private:
        // _fd 表示socket的描述符 为int类型
        int _fd;
        // 主机地址长度 
        socklen_t _addressLen;
        // 客户端地址长度
        socklen_t _peerAddressLen;
        // sockaddr_in 结构体是对sockaddr的ipv4类型封装
        struct sockaddr_in _sockAddress;
        struct sockaddr_in _peerAddress;
        // socket是否初始化
        bool _init;
        // 超时设置
        int _timeout;
    
    protected:
        unsigned int _getPort(sockaddr_in *addr);
        int _getAddress(sockaddr_in *addr, char *pAddress, unsigned int length);

    public:
        int setSocketLi(int lOnoFF, int linger);
        void setAddress(const char *pHostName, unsigned int port);
        // create a listening socket
        _ossSocket();
        _ossSocket(unsigned int port, int timeout=0);
        // create a connection socket
        _ossSocket(const char *pHostName, unsigned int port, int timeout=0);
        // create from existing socket
        _ossSocket(int *sock, int timeout=0);
        
        // 析构函数
        ~_ossSocket()
        {
            close();
        }

        int initSocket();
        int bind_listen();
        int send(const char *pMsg, int len,
                 int timeout=OSS_SOCKET_DFT_TIMEOUT,
                 int flags=0);
        // 接受指定长度的消息
        int recv(char *pMsg, int len,
                 int timeout=OSS_SOCKET_DFT_TIMEOUT,
                 int flags=0);
        // 只要接受到消息就处理
        int recvNF(char *pMsg, int len,
                   int timeout=OSS_SOCKET_DFT_TIMEOUT);
        
        int connect();
        void close();
        int accept(int *sock, struct sockaddr *addr, socklen_t *addrlen,
                   int timeout=OSS_SOCKET_DFT_TIMEOUT);

        // 辅助函数
        bool isConnected();
        // Nagle TCP 组合小包成大包，数据库通信只需发小包
        int disableNagle();
        unsigned int getLocalPort();
        unsigned int getPeerPort();
        int getPeerAddress(char *pAddress, unsigned int length);
        int getLocalAddress(char *pAddress, unsigned int length);
        int setTimeout(int seconds);
        static int getHostName(char *pName, int nameLen);
        static int getPort(const char *pServiceName, unsigned short &port);

};
typedef class _ossSocket ossSocket;

#endif