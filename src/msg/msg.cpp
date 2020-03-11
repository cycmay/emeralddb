#include "msg.hpp"
#include "core.hpp"
#include "pd.hpp"

using namespace bson;

static int msgCheckBuffer(char **ppBuffer, int *pBufferSize, int length)
{
	int rc = EDB_OK;
	if(length > *pBufferSize)
	{
		// 记录待检查buffer内存地址
		char *pOldBuf = *ppBuffer;
		if(length < 0)
		{
			PD_LOG(PDERROR, "Invalid length: %d", length);
			rc = EDB_INVALIDARG;
			goto error;
		}
		// 对缓冲区重新申请地址
		*ppBuffer = (char *)realloc(*ppBuffer, sizeof(char)*length);
		if(!ppBuffer)
		{
			// 注意realloc 失败则返回NULL 故将原始地址重新赋予之
			rc = EDB_OOM;
			*ppBuffer = pOldBuf;
			goto error;
		}
		*pBufferSize = length;
	}

error:
	goto done;
done:
	return rc;
}

// 打包reply数据（buffer, bsonobj）到MsgReply数据结构
int msgBuildReply(char **ppBuffer, int *pBufferSize, int returnCode,
					BSONObj *objReturn)
{
	int rc = EDB_OK;
	int size = sizeof(MsgReply);
	MsgReply * pReply = NULL;

	if(objReturn)
	{
		size += objReturn->objsize();
	} 

	rc = msgCheckBuffer(ppBuffer, pBufferSize, size);
	PD_RC_CHECK(rc, PDERROR, "Failed to realloc buffer for %d bytes, rc = %d.",
				size, rc);
	// 将缓冲区的数据打包给pReply
	// char ** ppBuffer 表示的是字符型数组的首地址的地址
	pReply = (MsgReply *)(*ppBuffer);
	// build header
	pReply->header.messageLen = size;
	pReply->header.opCode = OP_REPLY;
	// build body
	pReply->returnCode = returnCode;
	pReply->numReturn = (objReturn)?1:0;

	if(objReturn)
	{
		// 通过MsgRely数据结构中data[0]的地址，开始存储数据
		memcpy(&pReply->data[0], objReturn->objdata(), objReturn->objsize() );
	}

done:
	return rc;
error:
	goto done;

}

int msgExtractReply(char * pBuffer, int &returnCode, int &numReturn,
					const char **ppObjStart)
{
	int rc = EDB_OK;
	MsgReply * pReply = (MsgReply *)pBuffer;
	// sanity check for header
	if(pReply->header.messageLen < (int)sizeof(MsgReply))
	{
		PD_LOG(PDERROR, "Invalid length of reply message.");
		rc = EDB_INVALIDARG;
		goto error;
	}
	// sanity check for opCode
	if(pReply->header.opCode != OP_REPLY)
	{
		PD_LOG(PDERROR, "non-reply code is received: %d, expected: %d", 
					pReply->header.opCode, OP_REPLY);
		rc = EDB_INVALIDARG;
		goto error;
	}
	// extract 
	returnCode = pReply->returnCode;
	numReturn = pReply->numReturn;

	if(0==numReturn)
	{	
		// 传入的参数 const char ** ppObjectStart 表示一个地址用于保存字符串数据的起始地址，
		// 该地址不可变
		// 没有返回BSON数据包，令指向的地址为空NULL
		*ppObjStart = NULL;
	}
	else
	{
		// 有返回数据，令指向的地址为数据起始地址
		*ppObjStart = &(pReply->data[0]);
	}

done:
	return rc;
error:
	goto done;

}

int msgBuildInsert ( char **ppBuffer, int *pBufferSize, BSONObj &obj )
{
   int rc             = EDB_OK ;
   int size           = sizeof(MsgInsert) + obj.objsize() ;
   MsgInsert *pInsert = NULL ;
   rc = msgCheckBuffer ( ppBuffer, pBufferSize, size ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to realloc buffer for %d bytes, rc = %d",
               size, rc ) ;
      goto error ;
   }
   // buffer is allocated, let's assign variables
   pInsert                    = (MsgInsert*)(*ppBuffer) ;
   // build header
   pInsert->header.messageLen = size ;
   pInsert->header.opCode     = OP_INSERT ;
   // build body
   pInsert->numInsert         = 1 ;
   // bson object
   memcpy ( &pInsert->data[0], obj.objdata(), obj.objsize() ) ;
done :
   return rc ;
error :
   goto done ;
}

int msgBuildInsert ( char **ppBuffer, int *pBufferSize, vector<BSONObj*> &obj )
{
   int rc             = EDB_OK ;
   int size           = sizeof(MsgInsert) ;
   MsgInsert *pInsert = NULL ;
   vector<BSONObj*>::iterator it ;
   char *p            = NULL ;
   for ( it = obj.begin(); it != obj.end(); ++it )
   {
      size += (*it)->objsize() ;
   }
   rc = msgCheckBuffer ( ppBuffer, pBufferSize, size ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to realloc buffer for %d bytes, rc = %d",
               size, rc ) ;
      goto error ;
   }
   // buffer is allocated, let's assign variables
   pInsert                    = (MsgInsert*)(*ppBuffer) ;
   // build header
   pInsert->header.messageLen = size ;
   pInsert->header.opCode     = OP_INSERT ;
   // build body
   pInsert->numInsert         = obj.size() ;
   // bson object
   p = &pInsert->data[0] ;
   for ( it = obj.begin(); it != obj.end(); ++it )
   {
      memcpy ( p, (*it)->objdata(), (*it)->objsize() ) ;
      p += (*it)->objsize() ;
   }
done :
   return rc ;
error :
   goto done ;
}

int msgExtractInsert ( char *pBuffer, int &numInsert, const char **ppObjStart )
{
   int rc              = EDB_OK ;
   MsgInsert *pInsert  = (MsgInsert*)pBuffer ;
   // sanity check for header
   if ( pInsert->header.messageLen < (int)sizeof(MsgInsert) )
   {
      PD_LOG ( PDERROR, "Invalid length of insert message" ) ;
      rc = EDB_INVALIDARG ;
      goto error ;
   }
   // sanity check for opCode
   if ( pInsert->header.opCode != OP_INSERT )
   {
      PD_LOG ( PDERROR, "non-insert code is received: %d, expected %d",
               pInsert->header.opCode, OP_INSERT ) ;
      rc = EDB_INVALIDARG ;
      goto error ;
   }
   // extract
   numInsert  = pInsert->numInsert ;
   // object
   if ( 0 == numInsert )
   {
      *ppObjStart = NULL ;
   }
   else
   {
      *ppObjStart = &pInsert->data[0] ;
   }
done :
   return rc ;
error :
   goto done ;
}

int msgBuildDelete ( char **ppBuffer, int *pBufferSize, BSONObj &key )
{
   int rc             = EDB_OK ;
   int size           = sizeof(MsgDelete) + key.objsize() ;
   MsgDelete *pDelete = NULL ;
   rc = msgCheckBuffer ( ppBuffer, pBufferSize, size ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to realloc buffer for %d bytes, rc = %d",
               size, rc ) ;
      goto error ;
   }
   // buffer is allocated, let's assign variables
   pDelete                    = (MsgDelete*)(*ppBuffer) ;
   // build header
   pDelete->header.messageLen = size ;
   pDelete->header.opCode     = OP_DELETE ;
   // bson object
   memcpy ( &pDelete->key[0], key.objdata(), key.objsize() ) ;
done :
   return rc ;
error :
   goto done ;
}

int msgExtractDelete ( char *pBuffer, BSONObj &key )
{
   int rc              = EDB_OK ;
   MsgDelete *pDelete  = (MsgDelete*)pBuffer ;
   // sanity check for header
   if ( pDelete->header.messageLen < (int)sizeof(MsgDelete) )
   {
      PD_LOG ( PDERROR, "Invalid length of delete message" ) ;
      rc = EDB_INVALIDARG ;
      goto error ;
   }
   // sanity check for opCode
   if ( pDelete->header.opCode != OP_DELETE )
   {
      PD_LOG ( PDERROR, "non-delete code is received: %d, expected %d",
               pDelete->header.opCode, OP_DELETE ) ;
      rc = EDB_INVALIDARG ;
      goto error ;
   }
   // extract
   // object
   key = BSONObj ( &pDelete->key[0] ) ;
done :
   return rc ;
error :
   goto done ;
}

int msgBuildQuery ( char **ppBuffer, int *pBufferSize, BSONObj &key )
{
   int rc             = EDB_OK ;
   int size           = sizeof(MsgQuery) + key.objsize() ;
   MsgQuery *pQuery   = NULL ;
   rc = msgCheckBuffer ( ppBuffer, pBufferSize, size ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to realloc buffer for %d bytes, rc = %d",
               size, rc ) ;
      goto error ;
   }
   // buffer is allocated, let's assign variables
   pQuery                    = (MsgQuery*)(*ppBuffer) ;
   // build header
   pQuery->header.messageLen = size ;
   pQuery->header.opCode     = OP_QUERY ;
   // bson object
   memcpy ( &pQuery->key[0], key.objdata(), key.objsize() ) ;
done :
   return rc ;
error :
   goto done ;
}

int msgExtractQuery ( char *pBuffer, BSONObj &key )
{
   int rc              = EDB_OK ;
   MsgQuery *pQuery    = (MsgQuery*)pBuffer ;
   // sanity check for header
   if ( pQuery->header.messageLen < (int)sizeof(MsgQuery) )
   {
      PD_LOG ( PDERROR, "Invalid length of query message" ) ;
      rc = EDB_INVALIDARG ;
      goto error ;
   }
   // sanity check for opCode
   if ( pQuery->header.opCode != OP_QUERY )
   {
      PD_LOG ( PDERROR, "non-query code is received: %d, expected %d",
               pQuery->header.opCode, OP_QUERY ) ;
      rc = EDB_INVALIDARG ;
      goto error ;
   }
   // extract
   // object
   key = BSONObj ( &pQuery->key[0] ) ;
done :
   return rc ;
error :
   goto done ;
}

int msgBuildCommand ( char **ppBuffer, int *pBufferSize, BSONObj &obj )
{
   int rc               = EDB_OK ;
   int size             = sizeof(MsgCommand) + obj.objsize() ;
   MsgCommand *pCommand = NULL ;
   rc = msgCheckBuffer ( ppBuffer, pBufferSize, size ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to realloc buffer for %d bytes, rc = %d",
               size, rc ) ;
      goto error ;
   }
   // buffer is allocated, let's assign variables
   pCommand                    = (MsgCommand*)(*ppBuffer) ;
   // build header
   pCommand->header.messageLen = size ;
   pCommand->header.opCode     = OP_COMMAND ;
   // build body
   pCommand->numArgs           = 1 ;
   // bson object
   memcpy ( &pCommand->data[0], obj.objdata(), obj.objsize() ) ;
done :
   return rc ;
error :
   goto done ;
}

int msgBuildCommand ( char **ppBuffer, int *pBufferSize, vector<BSONObj*>&obj )
{
   int rc               = EDB_OK ;
   int size             = sizeof(MsgCommand) ;
   MsgCommand *pCommand = NULL ;
   vector<BSONObj*>::iterator it ;
   char *p            = NULL ;
   for ( it = obj.begin(); it != obj.end(); ++it )
   {
      size += (*it)->objsize() ;
   }
   rc = msgCheckBuffer ( ppBuffer, pBufferSize, size ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to realloc buffer for %d bytes, rc = %d",
               size, rc ) ;
      goto error ;
   }
   // buffer is allocated, let's assign variables
   pCommand                    = (MsgCommand*)(*ppBuffer) ;
   // build header
   pCommand->header.messageLen = size ;
   pCommand->header.opCode     = OP_COMMAND ;
   // build body
   pCommand->numArgs           = obj.size() ;
   // bson object
   p = &pCommand->data[0] ;
   for ( it = obj.begin(); it != obj.end(); ++it )
   {
      memcpy ( p, (*it)->objdata(), (*it)->objsize() ) ;
      p += (*it)->objsize() ;
   }
done :
   return rc ;
error :
   goto done ;
}

int msgExtractCommand ( char *pBuffer, int &numArgs, const char **ppObjStart )
{
   int rc                = EDB_OK ;
   MsgCommand *pCommand  = (MsgCommand*)pBuffer ;
   // sanity check for header
   if ( pCommand->header.messageLen < (int)sizeof(MsgCommand) )
   {
      PD_LOG ( PDERROR, "Invalid length of command message" ) ;
      rc = EDB_INVALIDARG ;
      goto error ;
   }
   // sanity check for opCode
   if ( pCommand->header.opCode != OP_COMMAND )
   {
      PD_LOG ( PDERROR, "non-command code is received: %d, expected %d",
               pCommand->header.opCode, OP_COMMAND ) ;
      rc = EDB_INVALIDARG ;
      goto error ;
   }
   // extract
   numArgs  = pCommand->numArgs ;
   // object
   if ( 0 == numArgs )
   {
      *ppObjStart = NULL ;
   }
   else
   {
      *ppObjStart = &pCommand->data[0] ;
   }
done :
   return rc ;
error :
   goto done ;
}
