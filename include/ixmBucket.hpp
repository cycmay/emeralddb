#ifndef IXMBUCKET_HPP__
#define IXMBUCKET_HPP__

#include "ossLatch.hpp"
#include "bson.h"
#include <map>
#include "dmsRecord.hpp"

using namespace bson;

#define IXM_KEY_FILEDNAME "_id"
#define IXM_HASH_MAP_SIZE 1000

struct ixmEleHash
{
	const char *data; // 指向的索引建
	dmsRecordID recordID; // 指向的数据

};

class ixmBucketManager
{
private:
	class ixmBucket
	{
	private:
		// the map is hashNum and eleHash
		std::multimap<unsigned int, ixmEleHash> _bucketMap;
		ossSLatch _mutex;
	public:
		// get the record whether exists
		int isIDExist(unsigned int hashNum, ixmEleHash &eleHash);
		int createIndex(unsigned int hashNum, ixmEleHash &eleHash);
		int findIndex(unsigned int hashNum, ixmEleHash &eleHash);
		int removeIndex(unsigned int hashNum, ixmEleHash &eleHash);
		
	};
	// process data
	int _processData(BSONObj &record, 
                        dmsRecordID &recordID, //in 
						unsigned int &hashNum,					// out
						ixmEleHash &eleHash, 					// out
						unsigned int &random);
private:
	std::vector<ixmBucket *>_bucket;
public:
	ixmBucketManager()
	{

	}
	~ixmBucketManager()
	{
		ixmBucket *pIxmBucket = NULL;
		for(int i = 0; i<IXM_HASH_MAP_SIZE; ++i)
		{
			pIxmBucket = _bucket[i];
			if(pIxmBucket)
				delete pIxmBucket;
		}
	}
	int initialize();
	int isIDExist(BSONObj &record);
	int createIndex(BSONObj &record, dmsRecordID &recordID);
	int findIndex(BSONObj &record, dmsRecordID &recordID);
	int removeIndex(BSONObj &record, dmsRecordID &recordID);

};


#endif
