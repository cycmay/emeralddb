#include "rtn.hpp"
#include "pd.hpp"
#include "core.hpp"
#include "pmd.hpp"


using namespace bson;

rtn::rtn():
_dmsFile(NULL),
_ixmBucketMgr(NULL)
{
}

rtn::~rtn()
{
	if(_ixmBucketMgr)
	{
		delete _ixmBucketMgr;
	}
	if(_dmsFile)
	{
		delete _dmsFile;
	}
}

int rtn::rtnInitialize()
{
	int rc = EDB_OK;
	_ixmBucketMgr = new(std::nothrow)ixmBucketManager();
	if(!_ixmBucketMgr)
	{
		rc = EDB_OOM;
		PD_LOG(PDERROR, "Failed to new ixmBucketManager");
		goto error;
	}
	_dmsFile = new(std::nothrow)dmsFile(_ixmBucketMgr);
	if(!_dmsFile)
	{
		rc = EDB_OOM;
		PD_LOG(PDERROR, "Failed to new dmsFile");
		goto error;
	}
	// init ixmBucketManager
	rc = _ixmBucketMgr->initialize();
	if(rc)
	{
		PD_LOG(PDERROR, "Failed to call ixmBucketManager initialize, rc = %d", rc);
		goto error;
	}
	// init dms
	rc = _dmsFile->initialize(pmdGetKRCB()->getDataFilePath());
	if(rc)
	{
		PD_LOG(PDERROR, "Failed to call dms initialize. rc = %d", rc);
		goto error;
	}

done:
	return rc;
error:
	goto done;
}

int rtn::rtnInsert(BSONObj &record)
{
	int rc = EDB_OK;
	dmsRecordID recordID;
	BSONObj outRecord;
	// check if _id exists
	rc = _ixmBucketMgr->isIDExist(record);
	PD_RC_CHECK(rc, PDERROR, "Failed to call ixmBucketManager isIDExist, rc = %d", rc);
	// whlie data insert into file
	rc = _dmsFile->insert(record, outRecord, recordID);
	if(rc)
	{
		PD_LOG(PDERROR, "Failed to call dms insert, rc = %d", rc);
		goto error;
	}
	rc = _ixmBucketMgr->createIndex(outRecord, recordID);
	PD_RC_CHECK(rc, PDERROR, "Failed to call ixmBucketManager createIndex, rc = %d", rc);
done:
	return rc;
error:
	goto done;
}

int rtn::rtnFind(BSONObj &inRecord, BSONObj &outRecord)
{
	int rc = EDB_OK;
	dmsRecordID recordID;
	rc = _ixmBucketMgr->findIndex(inRecord, recordID);
	PD_RC_CHECK(rc, PDERROR, "Failed to call ixm findIndex, rc = %d", rc);
	rc = _dmsFile->find(recordID, outRecord);
	PD_RC_CHECK(rc, PDERROR, "Failed to call dmsFile find, rc = %d", rc);

error:
	goto done;
done:
	return rc;
}

int rtn::rtnRemove(BSONObj &record)
{
	int rc = EDB_OK;
	dmsRecordID recordID;
	rc = _ixmBucketMgr->removeIndex(record, recordID);
	PD_RC_CHECK(rc, PDERROR, "Failed to call ixmBucketManager removeIndex, rc = %d", rc);
	rc = _dmsFile->remove(recordID);
	PD_RC_CHECK(rc, PDERROR, "Failed to call dmsFile remove, rc = %d", rc);

error:
	goto done;
done:
	return rc;
}
