#ifndef DMSRECORD_HPP__
#define DMSRECORD_HPP__

typedef unsigned int PAGEID;
typedef unsigned int SLOTID;

struct dmsRecordID
{
	PAGEID _pageID;
	SLOTID _slotID;
};

#endif