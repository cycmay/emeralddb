#include "core.hpp"
#include "pd.hpp"
#include "ossHash.hpp"
#include "ixmBucket.hpp"

int ixmBucketManager::isIDExist(BSONObj &record)
{
	int rc = EDB_OK;
	unsigned int hashNum = 0;
	unsigned int random = 0;
	ixmEleHash eleHash;
	dmsRecordID recordID;
	rc = _processData(record, recordID, hashNum, eleHash, random);
	if(rc)
	{
		PD_LOG(PDERROR, "Failed to process data, rc = %d",rc);
		goto error;
	}
	rc = _bucket[random]->isIDExist(hashNum, eleHash);
	if(rc)
	{
		PD_LOG(PDERROR, "Failed to create index, rc = %d", rc);
		goto error;
	}

error:
	goto done;
done:
	return rc;
}

int ixmBucketManager::createIndex(BSONObj &record, dmsRecordID &recordID)
{
	int rc = EDB_OK;
	unsigned int hashNum = 0;
	unsigned int random  = 0;
	ixmEleHash eleHash;
	rc = _processData(record, recordID, hashNum, eleHash, random);
	PD_RC_CHECK(rc, PDERROR, "Failed to process data. rc = %d", rc);
	rc = _bucket[random]->createIndex(hashNum, eleHash);
	PD_RC_CHECK(rc, PDERROR, "Failed to create index, rc = %d",rc);
	recordID = eleHash.recordID;
done:
	return rc;
error:
	goto done;
}

int ixmBucketManager::findIndex(BSONObj &record, dmsRecordID &recordID)
{
	int rc = EDB_OK;
	unsigned int hashNum = 0;
	unsigned int random  = 0;
	ixmEleHash eleHash;
	rc = _processData(record, recordID, hashNum, eleHash, random);
	PD_RC_CHECK(rc, PDERROR, "Failed to process data. rc = %d", rc);
	rc = _bucket[random]->findIndex(hashNum, eleHash);
	PD_RC_CHECK(rc, PDERROR, "Failed to find index, rc = %d", rc);
    recordID = eleHash.recordID;

error:
	goto done;
done:
	return rc;
}

int ixmBucketManager::removeIndex(BSONObj &record, dmsRecordID &recordID)
{
	int rc = EDB_OK;
	unsigned int hashNum = 0;
	unsigned int random  = 0;
	ixmEleHash eleHash;
	rc = _processData(record, recordID, hashNum, eleHash, random);
	PD_RC_CHECK(rc, PDERROR, "Failed to process data. rc = %d", rc);
	rc = _bucket[random]->removeIndex(hashNum, eleHash);
	PD_RC_CHECK(rc, PDERROR, "Failed to remove index, rc = %d", rc);
	recordID._pageID = eleHash.recordID._pageID;
	recordID._slotID = eleHash.recordID._slotID;

error:
	goto done;
done:
	return rc;
}

int ixmBucketManager::_processData(BSONObj &record, 
									dmsRecordID &recordID,
                                    unsigned int &hashNum,
									ixmEleHash &eleHash,
									unsigned int &random)
{
	int rc = EDB_OK;
	BSONElement element = record.getField(IXM_KEY_FILEDNAME);
	// check if _id exists and correct
	if(element.eoo() ||
		(element.type() != NumberInt && element.type() != String))
	{
		rc = EDB_INVALIDARG;
		PD_LOG(PDERROR, "record must be with _id");
		goto error;
	}
	// hash _id
	hashNum = ossHash(element.value(), element.valuesize());
	random = hashNum % IXM_HASH_MAP_SIZE;
	eleHash.data = element.rawdata();
	eleHash.recordID = recordID;
done:
	return rc;
error:
	goto done;
}

int ixmBucketManager::ixmBucket::isIDExist(unsigned int hashNum,
											ixmEleHash &eleHash)
{
	int rc = EDB_OK;
	BSONElement destEle;
	BSONElement sourEle;
	ixmEleHash 	exsitEle;
	std::pair<std::multimap<unsigned int, ixmEleHash>::iterator,
				std::multimap<unsigned int, ixmEleHash>::iterator> ret;
	
	// 对每个桶进行加锁
	_mutex.get_shared();
	// 检查桶内是否已经存在相同hash值的元素
	ret = _bucketMap.equal_range(hashNum);
	sourEle = BSONElement(eleHash.data);
	for(std::multimap<unsigned int, ixmEleHash>::iterator it = ret.first;
			it != ret.second; ++it)
	{
		exsitEle = it->second;
		destEle = BSONElement(exsitEle.data);
		if(sourEle.type() == destEle.type())
		{
			if(sourEle.valuesize() == destEle.valuesize())
			{
				if(!memcmp(sourEle.value(), destEle.value(),
							destEle.valuesize()))
				{
					rc = EDB_IXM_ID_EXIST;
					PD_LOG(PDERROR, "record _id:%d had existed", sourEle.value());
					goto error;
				}
			}
		}
	}
done:	
	_mutex.release_shared();
	return rc;
error:
	goto done;
}

int ixmBucketManager::ixmBucket::createIndex(unsigned int hashNum,
											ixmEleHash &eleHash)
{
	int rc = EDB_OK;
	_mutex.get();
   _bucketMap.insert (
      pair<unsigned int, ixmEleHash> ( hashNum, eleHash ) ) ;
   _mutex.release () ;
   return rc ;
}

int ixmBucketManager::ixmBucket::findIndex(unsigned int hashNum, 
											ixmEleHash &eleHash)
{
	int rc = EDB_OK;
	BSONElement destEle;
	BSONElement sourEle;
	ixmEleHash 	exsitEle;
	std::pair<std::multimap<unsigned int, ixmEleHash>::iterator,
				std::multimap<unsigned int, ixmEleHash>::iterator> ret;
	
	// 对每个桶进行加锁
	_mutex.get_shared();
	ret = _bucketMap.equal_range(hashNum);
	sourEle = BSONElement(eleHash.data);
	for(std::multimap<unsigned int, ixmEleHash>::iterator it = ret.first;
			it != ret.second; ++it)
	{
		exsitEle = it->second;
		destEle = BSONElement(exsitEle.data);
		if(sourEle.type() == destEle.type())
		{
			if(sourEle.valuesize() == destEle.valuesize())
			{
				if(!memcmp(sourEle.value(), destEle.value(),
							destEle.valuesize()))
				{
					eleHash.recordID = exsitEle.recordID;
					goto done;
				}
			}
		}
	}
	rc = EDB_IXM_ID_NOT_EXIST;
	PD_LOG(PDERROR, "record _id not exist, hashNum = %d", hashNum);
	goto error;

done:	
	_mutex.release_shared();
	return rc;
error:
	goto done;
}

int ixmBucketManager::ixmBucket::removeIndex(unsigned int hashNum,
											ixmEleHash &eleHash)
{
	int rc = EDB_OK;
	BSONElement destEle;
	BSONElement sourEle;
	ixmEleHash 	exsitEle;
	std::pair<std::multimap<unsigned int, ixmEleHash>::iterator,
				std::multimap<unsigned int, ixmEleHash>::iterator> ret;
	
	// 对每个桶进行加锁
	_mutex.get();
	ret = _bucketMap.equal_range(hashNum);
	sourEle = BSONElement(eleHash.data);
	for(std::multimap<unsigned int, ixmEleHash>::iterator it = ret.first;
			it != ret.second; ++it)
	{
		exsitEle = it->second;
		destEle = BSONElement(exsitEle.data);
		if(sourEle.type() == destEle.type())
		{
			if(sourEle.valuesize() == destEle.valuesize())
			{
				if(!memcmp(sourEle.value(), destEle.value(),
							destEle.valuesize()))
				{
					eleHash.recordID = exsitEle.recordID;
					_bucketMap.erase(it);
					goto done;
				}
			}
		}
	}
	rc = EDB_INVALIDARG;
	PD_LOG(PDERROR, "record _id not exist, rc = %d", rc);
	goto error;

done:	
	_mutex.release();
	return rc;
error:
	goto done;
}

int ixmBucketManager::initialize()
{
	int rc = EDB_OK;
	ixmBucket * temp = NULL;
	for(int i = 0; i < IXM_HASH_MAP_SIZE; ++i)
	{
		temp = new(std::nothrow)ixmBucket();
		if(!temp)
		{
			rc = EDB_OOM;
			PD_LOG(PDERROR, "Failed to allocate new ixmBucket, rc = %d", rc);
			goto error;
		}
		_bucket.push_back(temp);
		temp = NULL;
	}

done:
	return rc;
error:
	goto done;
}
