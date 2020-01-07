#include "core.hpp"
#include "command.hpp"
#include "commandFactory.hpp"

COMMAND_BEGIN   
COMMAND_ADD(COMMAND_CONNECT,ConnectCommand)
COMMAND_ADD(COMMAND_QUIT,QuitCommand)
COMMAND_ADD(COMMAND_HELP,HelpCommand)
COMMAND_END

extern int gQuit;

int ICommand::execute(ossSocket &sock, std::vector<std::string> &argVec)
{
    return EDB_OK;
}

int ICommand::getError(int code)
{
    switch(code)
    {
        case EDB_OK:
            break;
        case EDB_IO:
            std::cout<<"io error is occurred" <<std::endl;
            break;
        case EDB_INVALIDARG:
         std::cout << "invalid argument" << std::endl;
         break;
      case EDB_PERM:
         std::cout << "edb_perm" << std::endl;
         break;
      case EDB_OOM:
         std::cout << "edb_oom" << std::endl;
         break;
      case EDB_SYS:
         std::cout << "system error is occurred." << std::endl;
         break;
      case EDB_QUIESCED:
         std::cout << "EDB_QUIESCED" << std::endl;
         break;
      case EDB_NETWORK_CLOSE:
         std::cout << "net work is closed." << std::endl;
         break;
      case EDB_HEADER_INVALID:
         std::cout << "record header is not right." << std::endl;
         break;
      case EDB_IXM_ID_EXIST:
         std::cout << "record key is exist." << std::endl;
         break;
      case EDB_IXM_ID_NOT_EXIST:
         std::cout << "record is not exist" << std::endl;
         break;
      case EDB_NO_ID:
         std::cout << "_id is needed" << std::endl;
         break;
      case EDB_QUERY_INVALID_ARGUMENT:
         std::cout << "invalid query argument" << std::endl;
         break;
      case EDB_INSERT_INVALID_ARGUMENT:
         std::cout <<  "invalid insert argument" << std::endl;
         break;
      case EDB_DELETE_INVALID_ARGUMENT:
         std::cout << "invalid delete argument" << std::endl;
         break;
      case EDB_INVALID_RECORD:
         std::cout << "invalid record string" << std::endl;
         break;
      case EDB_SOCK_NOT_CONNECT:
         std::cout << "sock connection does not exist" << std::endl;
         break;
      case EDB_SOCK_REMOTE_CLOSED:
         std::cout << "remote sock connection is closed" << std::endl;
         break;
      case EDB_MSG_BUILD_FAILED:
         std::cout << "msg build failed" << std::endl;
         break;
      case EDB_SOCK_SEND_FAILED:
         std::cout << "sock send msg faild" << std::endl;
         break;
      case EDB_SOCK_INIT_FAILED:
         std::cout << "sock init failed" << std::endl;
         break;
      case EDB_SOCK_CONNECT_FAILED:
         std::cout << "sock connect remote server failed" << std::endl;
         break;
      default :
         break;
   }
   return code;
}

// receive a message that replyed
int ICommand::recvReply(ossSocket &sock)
{
    // define message data length
    int length = 0;
    int ret = EDB_OK;
    // fill receive buffer with 0.
    memset(_recvBuf, 0, RECV_BUF_SIZE);
    if(!sock.isConnected())
    {
        return getError(EDB_SOCK_NOT_CONNECT);
    }
    while(1)
    {
        // receive data from the server.first receive the length of the data
        ret = sock.recv(_recvBuf, sizeof(int));
        if(EDB_TIMEOUT == ret)
        {
            continue;
        }
        else if(EDB_NETWORK_CLOSE == ret)
        {
            return getError(EDB_SOCK_REMOTE_CLOSED);
        }
        else
        {
            break;
        }
    }
    // get the value of length
    length = *(int *)_recvBuf;
    if(length > RECV_BUF_SIZE)
    {
        return getError(EDB_RECV_DATA_LENGTH_ERROR);
    }

    // receive data from the server.second receive the last data
    while(1)
    {
        ret = sock.recv(&_recvBuf[sizeof(int)], length-sizeof(int));
        if(ret == EDB_TIMEOUT)
        {
            continue;
        }
        else if(EDB_NETWORK_CLOSE == ret)
        {
            return getError(EDB_SOCK_REMOTE_CLOSED);
        }
        else
        {
            break;
        }
    }
    return ret;
} 

int ICommand::sendOrder(ossSocket &sock, OnMsgBuild onMsgBuild)
{
    int ret = EDB_OK;
    bson::BSONObj bsonData;
    try
    {
        bsonData = bson::fromjson(_jsonString);
    }
    catch(const std::exception& e)
    {
        return getError(EDB_INVALID_RECORD);
    }
    memset(_sendBuf, 0, SEND_BUF_SIZE);
    int size = SEND_BUF_SIZE;
    char *pSendBuf = _sendBuf;
    ret = onMsgBuild(&pSendBuf, &size, bsonData);
    if(ret)
    {
        return getError(EDB_MSG_BUILD_FAILED);
    }
    ret = sock.send(pSendBuf, *(int *)pSendBuf);
    if(ret)
    {
        return getError(EDB_SOCK_SEND_FAILED);
    }
    return ret;
}

int ICommand::sendOrder(ossSocket &sock, int opCode)
{
    int ret = EDB_OK;
    memset(_sendBuf, 0, SEND_BUF_SIZE);
    char * pSendBuf = _sendBuf;
    const char *pStr = "hello world";
    *(int *)pSendBuf = strlen(pStr) + 1 + sizeof(int);
    memcpy(&pSendBuf[sizeof(int)], pStr, strlen(pStr)+1);
    ret = sock.send(pSendBuf, *(int *)pSendBuf);
}

/****************************Connect Command**********************************/
int ConnectCommand::execute(ossSocket &sock, std::vector<std::string> &argVec)
{
    int ret = EDB_OK;
    _address = argVec[0];
    _port = atoi(argVec[1].c_str());
    sock.close();
    sock.setAddress(_address.c_str(), _port);
    ret = sock.initSocket();
    if(ret)
    {
        return getError(EDB_SOCK_INIT_FAILED);
    }
    ret = sock.connect();
    if(ret)
    {
        return getError(EDB_SOCK_CONNECT_FAILED);
    }
    sock.disableNagle();
    return ret;
}

/****************************Quit Command**********************************/
int QuitCommand::handleReply()
{
    int ret = EDB_OK;
    // gQuit = 1;
    return ret;
}

int QuitCommand::execute(ossSocket &sock, std::vector<std::string> &argVec)
{
    int ret = EDB_OK;
    if(!sock.isConnected())
    {
        return getError(EDB_SOCK_NOT_CONNECT);
    }
    ret = sendOrder(sock, 0);
    // sock.close();
    ret = handleReply();
    return ret;
}

int HelpCommand::execute(ossSocket &sock, std::vector<std::string> &argVec)
{
    int ret = EDB_OK;
    printf("List of classes of commands:\n\n");
    printf("%s [server] [port]-- connecting emeralddb server\n", COMMAND_CONNECT);
    printf("%s -- sending a insert command to emeralddb server\n", COMMAND_INSERT);
    printf("%s -- sending a query command to emeralddb server\n", COMMAND_QUERY);
    printf("%s -- sending a delete command to emeralddb server\n", COMMAND_DELETE);
    printf("%s [number]-- sending a test command to emeralddb server\n", COMMAND_TEST);
    printf("%s -- providing current number of record inserting\n", COMMAND_SNAPSHOT);
    printf("%s -- quitting command\n\n", COMMAND_QUIT);
    printf("Type \"help\" command for help\n");
    return ret;
}