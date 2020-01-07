#include "ossSocket.hpp"
#include "pd.hpp"

// create a listening socket
_ossSocket::_ossSocket(unsigned int port, int timeout)
{
    _init = false;
    _fd = 0;
    _timeout = timeout;
    memset(&_sockAddress, 0, sizeof(sockaddr_in));
    memset(&_peerAddress, 0, sizeof(sockaddr_in));
    _peerAddressLen = sizeof(_peerAddress);
    _sockAddress.sin_family = AF_INET;
    _sockAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    _sockAddress.sin_port = htons(port);
    _addressLen = sizeof(_sockAddress);
}

// create a socket 全新的套接字
_ossSocket::_ossSocket()
{
    _init = false;
    _fd = 0;
    _timeout = 0;
    memset(&_sockAddress, 0, sizeof(sockaddr_in));
    memset(&_peerAddress, 0, sizeof(sockaddr_in));
    _peerAddressLen = sizeof(_peerAddress);
    _addressLen = sizeof(_sockAddress);
}

// create a connecting socket   创建客户端的套接字 不同于本地创建的监听的套接字
_ossSocket::_ossSocket(const char *pHostName, unsigned int port, int timeout)
{
    struct hostent *hp;
    _init = false;
    _timeout = timeout;
    _fd = 0;
    memset(&_sockAddress, 0, sizeof(sockaddr_in));
    memset(&_peerAddress, 0, sizeof(sockaddr_in));
    _peerAddressLen = sizeof(_peerAddress);
    _sockAddress.sin_family = AF_INET;
    // 解析对方的机型
    if( (hp=gethostbyname(pHostName)) )
    {
        _sockAddress.sin_addr.s_addr = *((int *)hp->h_addr_list[0]);
    }else
    {
        _sockAddress.sin_addr.s_addr = inet_addr(pHostName);
    }
    _sockAddress.sin_port = htons(port);
    _addressLen = sizeof(_sockAddress);
}

// create from a existing socket
_ossSocket::_ossSocket(int *sock, int timeout)
{
    int rc = EDB_OK;
    _fd = *sock;
    // 已经存在的socket表示已经初始化过
    _init = true;
    _timeout = timeout;
    _addressLen = sizeof(_sockAddress);
    memset(&_peerAddress, 0 , sizeof(sockaddr_in));
    _peerAddressLen = sizeof(_peerAddress);
    rc = getsockname(_fd, (sockaddr*)&_sockAddress, &_addressLen);
    if  (rc)
    {
        PD_LOG(PDERROR, "Failed to get sock name, error = %d", 
                SOCKET_GETLASTERROR);
        _init = false;
    }else
    {
        rc = getpeername(_fd, (sockaddr *)&_peerAddress, &_peerAddressLen);
        PD_RC_CHECK(rc, PDERROR, "Failed to get peer name, error = %d",
                    SOCKET_GETLASTERROR);
    }
done:
    return;
error:
    goto done;
}

int _ossSocket::initSocket()
{
    int rc = EDB_OK;
    if(_init)
    {
        goto done;
    }
    memset(&_peerAddress, 0, sizeof(sockaddr_in));
    _peerAddressLen = sizeof(_peerAddress);
    _fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(-1 == _fd)
    {
        PD_RC_CHECK(EDB_NETWORK, PDERROR, "Failed to initialize socket, error = %d",
                SOCKET_GETLASTERROR);
    }
    _init = true;
    // set timeout
    setTimeout(_timeout);

done :
    return rc;
error :
    goto done;
}

int _ossSocket::setSocketLi(int lOnOff, int linger)
{
    int rc = EDB_OK;
    struct linger _linger;
    _linger.l_onoff = lOnOff;
    _linger.l_linger = linger;
    rc = setsockopt(_fd, SOL_SOCKET, SO_LINGER,
                    (const char *)&_linger, sizeof(_linger));
    return rc;
}

void _ossSocket::setAddress(const char *pHostName, unsigned int port)
{
    struct hostent *hp;
    memset(&_sockAddress, 0, sizeof(sockaddr_in));
    memset(&_peerAddress, 0, sizeof(sockaddr_in));
    _peerAddressLen = sizeof(_peerAddress);
    _sockAddress.sin_family = AF_INET;  // IPV4
    //获取hostent结构体
    if( (hp=gethostbyname(pHostName)) )
        _sockAddress.sin_addr.s_addr = *((int *)hp->h_addr_list[0]);
    else
        _sockAddress.sin_addr.s_addr = inet_addr(pHostName);

    _sockAddress.sin_port = htons(port);
    _addressLen = sizeof(_sockAddress);
}

int _ossSocket::bind_listen()
{
    int rc = EDB_OK;
    int temp = 1;
    rc = setsockopt(_fd, SOL_SOCKET,
                    SO_REUSEADDR, (char *)&temp, sizeof(int));
    if(rc)
    {
        PD_LOG(PDWARNING, "Failed to setsocktopt SO_REUSEADDR, error = %d",
                SOCKET_GETLASTERROR);
    }
    rc = setSocketLi(1,30);
    if (rc)
    {   
        PD_LOG(PDWARNING, "Failed to setsockopt SO_LINGER, error = %d", 
                SOCKET_GETLASTERROR);
    }
    rc = ::bind(_fd, (struct sockaddr*)&_sockAddress, _addressLen);
    if(rc)
    {
        PD_RC_CHECK(EDB_NETWORK, PDERROR, "Failed to bind socket, error = %d", 
                SOCKET_GETLASTERROR);
    }
    rc = listen(_fd, SOMAXCONN);
    if(rc)
    {   
        PD_RC_CHECK(EDB_NETWORK, PDERROR, "Failed to listen socket, error = %d",
                SOCKET_GETLASTERROR);
    }

done:
    return rc;
error:
    close();
    goto done;
}

int _ossSocket::send(const char *pMsg, int len,
                    int timeout, int flags)
{
    int rc = EDB_OK;
    int maxFD = _fd;
    struct timeval maxSelectTime;
    fd_set fds;

    maxSelectTime.tv_sec = timeout / 1000000;
    maxSelectTime.tv_usec = timeout % 1000000;
    // if len==0 then let's just return
    if(0==len)
    {
        return EDB_OK;
    }
    // wait loop until socket is ready
    while(true)
    {
        // 清空
        FD_ZERO(&fds);
        // add
        FD_SET(_fd, &fds);
        rc = select(maxFD + 1, NULL, &fds, NULL,
                    timeout>=0?&maxSelectTime:NULL);
        if(0==rc)
        {
            // timeout
            rc = EDB_TIMEOUT;
            goto done;
        }
        // if rf < 0, someting wrong
        if(0>rc)
        {
            rc = SOCKET_GETLASTERROR;
            // failed due to interrupt, let's continue
            if(EINTR==rc)
            {
                continue;
            }
            PD_RC_CHECK(EDB_NETWORK, PDERROR, "Failed to select from socket, error = %d",
                    SOCKET_GETLASTERROR);
        }
        // FD_ISSET() tests to see if a file descriptor is part of the set; 
        if(FD_ISSET(_fd, &fds))
        {
            break;
        }
    }
    // 发送消息
    while(len>0)
    {
        // MSG_NOSIGNAL: Requests not to send SIGPIPE on errors on stream oriented sockets
        // when the other end breaks the connetction. The EPIPE error is still returned
        rc = ::send(_fd, pMsg, len, MSG_NOSIGNAL|flags);
        if (-1 == rc)
        {
            PD_RC_CHECK(EDB_NETWORK, PDERROR, "Failed to send, error = %d", 
                        SOCKET_GETLASTERROR);
        }
        len -=rc;
        pMsg += rc;
    }
    rc = EDB_OK;

done:
    return rc;
error:
    goto done;
}

bool _ossSocket::isConnected()
{
    int rc =  EDB_OK;
    rc = ::send(_fd, "", 0, MSG_NOSIGNAL);
    if(0>rc)
        return false;
    else
        return true;
}

#define MAX_RECV_PETRIES 5
int _ossSocket::recv(char *pMsg, int len,
                    int timeout, int flags)
{
    int rc = EDB_OK;
    int retries = 0;
    int maxFD = _fd;
    struct timeval maxSelectTime;
    fd_set fds;

    if(0==len)
        return EDB_OK;
    maxSelectTime.tv_sec = timeout/1000000;
    maxSelectTime.tv_usec = timeout%1000000;
    while(true)
    {
        FD_ZERO(&fds);
        FD_SET(_fd, &fds);
        rc = select(maxFD+1, &fds, NULL, NULL,
                    timeout>=0?&maxSelectTime:NULL);
        // 0 means timeout
        if(0==rc)
        {
            rc = EDB_TIMEOUT;
            goto done;
        }
        // if < 0 , something wrong
        if(0>rc)
        {
            rc = SOCKET_GETLASTERROR;
            if(EINTR==rc)
            {
                continue;
            }
            PD_RC_CHECK(EDB_NETWORK, PDERROR, 
                        "Failed to select from socket, error = %d", 
                        rc);
        }
        if(FD_ISSET(_fd, &fds))
        {
            break;
        }
    }
    while(len > 0 )
    {
        rc = ::recv(_fd, pMsg, len, MSG_NOSIGNAL|flags);
        if(rc > 0)
        {
            if(flags & MSG_PEEK)
            {
                goto done;
            }
            len -= rc;
            pMsg += rc;
        }
        else if(rc==0)
        {
            PD_RC_CHECK(EDB_NETWORK_CLOSE, PDINFO, 
                        "Peer unexcepted shutdown");
        }
        else
        {
            rc = SOCKET_GETLASTERROR;
            if((EAGAIN==rc||EWOULDBLOCK==rc) &&
                _timeout>0)
            {
                PD_RC_CHECK(EDB_NETWORK, PDERROR, 
                            "Recv() timeout, error = %d", rc);
            }
            if((EINTR == rc) && (retries<MAX_RECV_PETRIES))
            {
                retries++;
                continue;
            }
            PD_RC_CHECK(EDB_NETWORK, PDERROR, 
                        "Recv() Failed, error = %d", rc);
        }
    }
    rc = EDB_OK;
done:
    return rc;
error:
    goto done;
}

int ossSocket::recvNF(char *pMsg, int len,
                    int timeout)
{
    int rc = EDB_OK;
    int retries = 0;
    int maxFD = _fd;
    struct timeval maxSelectTime;
    fd_set fds;
    // if we don't expect to receive anything, no need to continue.
    if(0==len)
        return EDB_OK;
    maxSelectTime.tv_sec = timeout/1000000;
    maxSelectTime.tv_usec = timeout%1000000;
    while(true)
    {
        FD_ZERO(&fds);
        FD_SET(_fd, &fds);
        rc = select(maxFD+1, &fds, NULL, NULL,
                    timeout>=0?&maxSelectTime:NULL);
        // 0 means timeout
        if(0==rc)
        {
            rc = EDB_TIMEOUT;
            goto done;
        }
        // if < 0 , something wrong
        if(0>rc)
        {
            rc = SOCKET_GETLASTERROR;
            if(EINTR==rc)
            {
                continue;
            }
            PD_RC_CHECK(EDB_NETWORK, PDERROR, 
                        "Failed to select from socket, error = %d", rc);
        }
        if(FD_ISSET(_fd, &fds))
        {
            break;
        }
    }
    rc = ::recv(_fd, pMsg, len, MSG_NOSIGNAL);
    if(rc > 0)
    {
        len = rc;
    }
    else if(rc==0)
    {
        PD_RC_CHECK(EDB_NETWORK_CLOSE, PDERROR, "Peer unexcepted shutdown");
    }
    else
    {
        rc = SOCKET_GETLASTERROR;
        if((EAGAIN==rc||EWOULDBLOCK==rc) &&
            _timeout>0)
        {
            PD_RC_CHECK(EDB_NETWORK, PDERROR, 
                        "Recv() timeout, error = %d", rc);
        }
        if((EINTR == rc) && (retries<MAX_RECV_PETRIES))
        {
            retries++;
        }
        PD_RC_CHECK(EDB_NETWORK, PDERROR, 
                    "Recv() Failed, error = %d", rc);
    }
    rc = EDB_OK;
done:
    return rc;
error:
    goto done;
}

int ossSocket::connect()
{
    int rc = EDB_OK;
    rc = ::connect(_fd, (struct sockaddr *)&_sockAddress, _addressLen);
    if(rc)
    {
        PD_RC_CHECK(EDB_NETWORK, PDERROR, 
                    "Failed to connect, error = %d", SOCKET_GETLASTERROR);
    }
    rc = getsockname(_fd, (sockaddr*)&_sockAddress, &_addressLen);
    if (rc)
    {
        PD_RC_CHECK(EDB_NETWORK, PDERROR,
                    "Failed to get local address, error = %d", rc);
    }
    // get peer address
    rc = getpeername(_fd, (sockaddr*)&_peerAddress, &_peerAddressLen);
    if(rc)
    {
        PD_RC_CHECK(EDB_NETWORK, PDERROR,
                    "Failed to get peer address, error = %d", rc);
    }

done:
    return rc;
error:
    goto done;
}

void _ossSocket::close()
{
    if(_init)
    {
        int i=0;
        i = ::close(_fd);
        if(i<0)
            i = -1;
        _init = false;
    }
}

int _ossSocket::accept(int *sock, struct sockaddr *addr, socklen_t *addrlen,
                      int timeout)
{
    int rc = EDB_OK;
    int maxFD = _fd;
    struct timeval maxSelectTime;
    fd_set fds;
    maxSelectTime.tv_sec = timeout/1000000;
    maxSelectTime.tv_usec = timeout%1000000;
    while(true)
    {
        FD_ZERO(&fds);
        FD_SET(_fd, &fds);
        rc = select(maxFD+1, &fds, NULL, NULL,
                    timeout>=0?&maxSelectTime:NULL);
        // 0 means timeout
        if(0==rc)
        {
            rc = EDB_TIMEOUT;
            goto done;
        }
        // if < 0 , something wrong
        if(0>rc)
        {
            rc = SOCKET_GETLASTERROR;
            if(EINTR==rc)
            {
                continue;
            }
            PD_RC_CHECK(EDB_NETWORK, PDERROR, 
                        "Failed to select from socket, error = %d", rc);
        }
        if(FD_ISSET(_fd, &fds))
        {
            break;
        }
    }
    rc = EDB_OK;
    *sock = ::accept(_fd, addr, addrlen);
    if(-1 == *sock)
    {
        PD_RC_CHECK(EDB_NETWORK, PDERROR, 
                    "Failed to accept socket, error = %d", SOCKET_GETLASTERROR);
    }
done:
    return rc;
error:
    goto done;
}

unsigned int _ossSocket::_getPort(sockaddr_in *addr)
{
    return ntohs(addr->sin_port);
}

int _ossSocket::_getAddress(sockaddr_in *addr, char *pAddress, unsigned int length)
{
    int rc = EDB_OK;
    length = length < NI_MAXHOST ? length : NI_MAXHOST;
    rc = getnameinfo((struct sockaddr*)addr, sizeof(sockaddr),  pAddress, length,
                    NULL, 0, NI_NUMERICHOST);
    if(rc)
    {
        PD_RC_CHECK(EDB_NETWORK, PDERROR,
                    "Failed to getnameinfo, error = %d", rc);
    }
done:
    return rc;
error:
    goto done;
}

unsigned int _ossSocket::getLocalPort()
{
    return _getPort(&_sockAddress);
}

unsigned int _ossSocket::getPeerPort()
{
    return _getPort(&_peerAddress);
}

int _ossSocket::getLocalAddress(char * pAddress, unsigned int length)
{
    return _getAddress(&_sockAddress, pAddress, length);
}

int _ossSocket::getPeerAddress(char * pAddress, unsigned int length)
{
    return _getAddress(&_peerAddress, pAddress, length);
}

int _ossSocket::setTimeout(int seconds)
{
    int rc  =EDB_OK;
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    // windows take milliseconds as parameter
    // linux takes timeval as input
    rc = setsockopt(_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
    if(rc)
    {
        PD_LOG(PDERROR, "Failed to setsockopt, error = %d", SOCKET_GETLASTERROR);
    }
    rc = setsockopt(_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
    if(rc)
    {
        PD_LOG(PDERROR, "Failed to setsockopt, error = %d", SOCKET_GETLASTERROR);
    }
    return rc;
}

int _ossSocket::getHostName(char *pName, int nameLen)
{
    return gethostname(pName, nameLen);
}

int _ossSocket::getPort(const char *pServiceName, unsigned short &port)
{
    int rc = EDB_OK;
    struct servent *servinfo;
    servinfo = getservbyname(pServiceName, "tcp");
    if(!servinfo)
        port = atoi(pServiceName);
    else
        port = (unsigned short) ntohs(servinfo->s_port);
    return rc;
}

int _ossSocket::disableNagle ()
{
   int rc = EDB_OK ;
   int temp = 1 ;
   rc = setsockopt ( _fd, IPPROTO_TCP, TCP_NODELAY, (char *) &temp,
                     sizeof ( int ) ) ;
   if ( rc )
   {
      PD_LOG ( PDWARNING, "Failed to setsockopt, rc = %d", SOCKET_GETLASTERROR ) ;
   }

   rc = setsockopt ( _fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &temp,
                     sizeof ( int ) ) ;
   if ( rc )
   {
      PD_LOG ( PDWARNING, "Failed to setsockopt, rc = %d", SOCKET_GETLASTERROR ) ;
   }
   return rc ;
}