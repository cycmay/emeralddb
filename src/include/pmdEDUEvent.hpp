// 事件通讯的机制
#ifndef PMDEDUEVENT_HPP__
#define PMDEDUEVENT_HPP__
#include "core.hpp"

enum pmdEDUEventTypes
{
    PMD_EDU_EVENT_NONE = 0,
    PMD_EDU_EVENT_TERM, // terminate EDU
    PMD_EDU_EVENT_RESUME, // resume a EDU, the data is start EDU's argv
    PMD_EDU_EVENT_ACTIVE,
    PMD_EDU_EVENT_DEACTIVE,
    PMD_EDU_EVENT_MSG,
    PMD_EDU_EVENT_TIMEOUT,  // timeout
    PMD_EDU_EVENT_LOCKWAKEUP    // transaction lock wake up
};

class pmdEDUEvent
{
    public:
        pmdEDUEvent():
            _eventType(PMD_EDU_EVENT_NONE),
            _release(false),
            _Data(NULL)
        {

        }
        pmdEDUEvent(pmdEDUEventTypes type):
            _eventType(type),
            _release(false),
            _Data(NULL)
        {

        }
        pmdEDUEvent(pmdEDUEventTypes type, bool release, void *data):
            _eventType(type),
            _release(release),
            _Data(data)
        {

        }

        void reset()
        {
            _eventType = PMD_EDU_EVENT_NONE;
            _release = false;
            _Data = NULL;
        }
        pmdEDUEventTypes     _eventType;
        bool                 _release;
        void                *_Data;

};

#endif
