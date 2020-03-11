#ifndef MSG_HPP__
#define MSG_HPP__

#include "../bson/src/bson.h"

#define OP_REPLY			1
#define OP_INSERT			2
#define OP_DELETE			3
#define OP_QUERY			4
#define OP_COMMAND			5
#define OP_DISCONNECT		6
#define OP_CONNECT     		7
#define OP_SNAPSHOT			8

#define RETURN_CODE_STATE_OK	1

struct MsgHeader
{
	int messageLen;
	int opCode;

};

struct MsgReply
{
	MsgHeader 		header;
	int 			returnCode;
	int 			numReturn;
	char			data[0];
};

struct MsgInsert
{
	MsgHeader		header;
	int 			numInsert;
	char			data[0];
};

struct MsgDelete
{
	MsgHeader 		header;
	char			key[0];
};

struct MsgQuery
{
	MsgHeader		header;
	char 			key[0];
};

struct MsgCommand
{
	MsgHeader		header;
	int 			numArgs;
	char 			data[0];
};

// 打包数据（buffer, bsonobj）到MsgReply数据结构
int msgBuildReply(char **ppBuffer, int *pBufferSize, int returnCode, 
					bson::BSONObj *objReturn);
// 解压MsgReply数据结构内容到内存（buffer, bsonobj）
int msgExtractReply(char **ppBuffer, int &returnCode, int &numReturn, 
					const char **ppObjectStart);

int msgBuildInsert(char **ppBuffer, int *pBufferSize, bson::BSONObj &obj);

int msgBuildInsert ( char **ppBuffer, int *pBufferSize, vector<bson::BSONObj*> &obj ) ;

int msgExtractInsert ( char *pBuffer, int &numInsert, const char **ppObjStart ) ;

int msgBuildDelete ( char **ppBuffer, int *pBufferSize, bson::BSONObj &key ) ;

int msgExtractDelete  ( char *pBuffer, bson::BSONObj &key ) ;

int msgBuildQuery ( char **ppBuffer, int *pBufferSize, bson::BSONObj &key ) ;

int msgExtractQuery ( char *pBuffer, bson::BSONObj &key ) ;

int msgBuildCommand ( char **ppBuffer, int *pBufferSize, bson::BSONObj &obj ) ;

int msgBuildCommand ( char **ppBuffer, int *pBufferSize, vector<bson::BSONObj*>&obj ) ;

int msgExtractCommand ( char *pBuffer, int &numArgs, const char **ppObjStart ) ;

#endif
