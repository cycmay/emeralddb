#include "core.hpp"
#include "command.hpp"
#include "commandFactory.hpp"
#include "pd.hpp"
#include "msg.hpp"

COMMAND_BEGIN   
COMMAND_ADD(COMMAND_INSERT, InsertCommand)
COMMAND_ADD(COMMAND_QUERY, QueryCommand)
COMMAND_ADD(COMMAND_DELETE, DeleteCommand)
COMMAND_ADD(COMMAND_CONNECT,ConnectCommand)
COMMAND_ADD(COMMAND_QUIT,QuitCommand)
COMMAND_ADD(COMMAND_HELP,HelpCommand)
COMMAND_ADD(COMMAND_SNAPSHOT, SnapshotCommand)
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
  // OnMsgBuild 是一个函数指针，指向的是要执行的函数
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
    // _sendBuf 是类Command的一块私有内存空间 用于缓冲将要发送的字节
    // onMsgBuild 是用户自定义的函数，不同的命令发送不同的函数。主要功能都是将数据打包成MsgReply数据结构并保存到
    // pSendBuf指向的内存空间
    ret = onMsgBuild(&pSendBuf, &size, bsonData);
    if(ret)
    {
        return getError(EDB_MSG_BUILD_FAILED);
    }
    // 发送pSendBuf的数据，*(int *)pSendBuf是前int个字节 表示数据包的长度
    ret = sock.send(pSendBuf, *(int *)pSendBuf);
    if(ret)
    {
        return getError(EDB_SOCK_SEND_FAILED);
    }
    return ret;
}


// 发送操作指令
int ICommand::sendOrder(ossSocket &sock, int opCode)
{
    int ret = EDB_OK;
    memset(_sendBuf, 0, SEND_BUF_SIZE);

    char * pSendBuf     = _sendBuf;
    MsgHeader *header   = (MsgHeader *)pSendBuf;
    header->messageLen  = sizeof(MsgHeader);
    header->opCode      = opCode;

    ret = sock.send(pSendBuf, *(int *)pSendBuf);
    return ret;
}


/******************************InsertCommand**********************************************/
int InsertCommand::handleReply()
{
   MsgReply * msg = (MsgReply*)_recvBuf;
   int returnCode = msg->returnCode;
   int ret = getError(returnCode);
   return ret;
}

int InsertCommand::execute( ossSocket & sock, std::vector<std::string> & argVec )
{
   int rc = EDB_OK;
   if( argVec.size() <1 )
   {
      return getError(EDB_INSERT_INVALID_ARGUMENT);
   }
   _jsonString = argVec[0];
     if( !sock.isConnected() )
   {
      return getError(EDB_SOCK_NOT_CONNECT);
   }

   rc = sendOrder( sock, msgBuildInsert );
   PD_RC_CHECK ( rc, PDERROR, "Failed to send order, rc = %d", rc ) ;

   rc = recvReply( sock );
   PD_RC_CHECK ( rc, PDERROR, "Failed to receive reply, rc = %d", rc ) ;
   rc = handleReply();
   PD_RC_CHECK ( rc, PDERROR, "Failed to receive reply, rc = %d", rc ) ;
done :
   return rc;
error :
   goto done ;
}

// QUERY
/******************************QueryCommand**********************************************/
int QueryCommand::handleReply()
{
  // 处理接收到的数据，数据在_recvBuf, 根据通信协议的数据结构MsgReply进行解析
   MsgReply * msg = (MsgReply*)_recvBuf;
   int returnCode = msg->returnCode;
   int ret = getError(returnCode);
   if(ret)
   {
      return ret;
   }
   if ( msg->numReturn )
   {
      bson::BSONObj bsonData = bson::BSONObj( &(msg->data[0]) );
      std::cout << bsonData.toString() << std::endl;
   }
   return ret;
}

int QueryCommand::execute( ossSocket & sock, std::vector<std::string> & argVec )
{
   int rc = EDB_OK;
   if( argVec.size() <1 )
   {
      return getError(EDB_QUERY_INVALID_ARGUMENT);
   }
   _jsonString = argVec[0];
   if( !sock.isConnected() )
   {
      return getError(EDB_SOCK_NOT_CONNECT);
   }

   rc = sendOrder( sock, msgBuildQuery );
   PD_RC_CHECK ( rc, PDERROR, "Failed to send order, rc = %d", rc ) ;
   rc = recvReply( sock );
   PD_RC_CHECK ( rc, PDERROR, "Failed to receive reply, rc = %d", rc ) ;
   rc = handleReply();
   PD_RC_CHECK ( rc, PDERROR, "Failed to receive reply, rc = %d", rc ) ;
done :
   return rc;
error :
   goto done ;
}

/******************************DeleteCommand**********************************************/
int DeleteCommand::handleReply()
{
   MsgReply * msg = (MsgReply*)_recvBuf;
   int returnCode = msg->returnCode;
   int ret = getError(returnCode);
   return ret;
}

int DeleteCommand::execute( ossSocket & sock, std::vector<std::string> & argVec )
{
   int rc = EDB_OK;
   if( argVec.size() < 1 )
   {
      return getError(EDB_DELETE_INVALID_ARGUMENT);
   }
   _jsonString = argVec[0];
   if( !sock.isConnected() )
   {
      return getError(EDB_SOCK_NOT_CONNECT);
   }
   rc = sendOrder( sock, msgBuildDelete );
   PD_RC_CHECK ( rc, PDERROR, "Failed to send order, rc = %d", rc ) ;
   rc = recvReply( sock );
   PD_RC_CHECK ( rc, PDERROR, "Failed to receive reply, rc = %d", rc ) ;
   rc = handleReply();
   PD_RC_CHECK ( rc, PDERROR, "Failed to receive reply, rc = %d", rc ) ;
done :
   return rc;
error :
   goto done ;
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
    gQuit = 1;
    return ret;
}

int QuitCommand::execute(ossSocket &sock, std::vector<std::string> &argVec)
{
    int ret = EDB_OK;
    if(!sock.isConnected())
    {
        return getError(EDB_SOCK_NOT_CONNECT);
    }
    ret = sendOrder(sock, OP_DISCONNECT);
    sock.close();
    ret = handleReply();
    return ret;
}

/****************************Help Command**********************************/

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

/*********************************SnapshotCommand******************************************/
int SnapshotCommand::handleReply()
{
   int ret = EDB_OK;
   MsgReply * msg = (MsgReply*)_recvBuf;
   int returnCode = msg->returnCode;
   ret = getError(returnCode);
   if(ret)
   {
      return ret;
   }
   bson::BSONObj bsonData = bson::BSONObj( &(msg->data[0]) );
   printf( "insert times is %d\n", bsonData.getIntField("insertTimes") );
   printf( "del times is %d\n", bsonData.getIntField("delTimes") );
   printf( "query times is %d\n", bsonData.getIntField("queryTimes") );
   printf( "server run time is %dm\n", bsonData.getIntField("serverRunTime") );

   return ret;
}

int SnapshotCommand::execute( ossSocket & sock, std::vector<std::string> &argVec)
{
   int rc = EDB_OK;
   if( !sock.isConnected() )
   {
      return getError(EDB_SOCK_NOT_CONNECT);
   }

   rc = sendOrder( sock, OP_SNAPSHOT );
   PD_RC_CHECK ( rc, PDERROR, "Failed to send order, rc = %d", rc ) ;
   rc = recvReply( sock );
   PD_RC_CHECK ( rc, PDERROR, "Failed to receive reply, rc = %d", rc ) ;
   rc = handleReply();
   PD_RC_CHECK ( rc, PDERROR, "Failed to receive reply, rc = %d", rc ) ;
done :
   return rc;
error :
   goto done ;
}
