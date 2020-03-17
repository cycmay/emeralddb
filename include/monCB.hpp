#ifndef MONCB_HPP__
#define MONCB_HPP__

#include <time.h>
#include "ossLatch.hpp"

class MonAppCB
{
public:
	MonAppCB();
	~MonAppCB();

	void setInsertTimes(long long insertTimes);
	long long getInsertTimes() const;
	void increaseInsertTimes();
	void setDelTimes(long long delTimes);
	long long getDelTimes() const;
	void increaseDelTimes();
	void setQueryTimes(long long queryTimes);
	long long getQueryTimes() const;
	void increaseQueryTimes();
	long long getServerRunTime();

private:
	long long _insertTimes;
	long long _delTimes;
	long long _queryTimes;
	struct timeval _start;
	ossSLatch _mutex;
};

#endif