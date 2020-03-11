#include "pmdEDUMgr.hpp"    // 线程池
#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "pd.hpp"

static std::map<EDU_TYPES, std::string> mapEDUName;
static std::map<EDU_TYPES, EDU_TYPES>   mapEDUTypeSys;

int registerEDUName(EDU_TYPES type, const char *name, bool system)
{
    int rc = EDB_OK;
    std::map<EDU_TYPES, std::string>::iterator it =
        mapEDUName.find(type);
    if(it!=mapEDUName.end())
    {
        PD_LOG(PDERROR, "EDU Type conflict[type:%d, %s<->%s]",
            (int)type, it->second.c_str(), name);
        rc = EDB_SYS;
        goto error;
    }
    mapEDUName[type] = std::string(name);
    if(system)
    {
        mapEDUTypeSys[type] = type;
    }
error:
    goto done;
done:
    return rc;
}
const char *getEDUName ( EDU_TYPES type )
{
   std::map<EDU_TYPES, std::string>::iterator it =
         mapEDUName.find ( type ) ;
   if ( it != mapEDUName.end() )
   {
      return it->second.c_str() ;
   }
   return "Unknown" ;
}

bool isSystemEDU ( EDU_TYPES type )
{
   std::map<EDU_TYPES,EDU_TYPES>::iterator it = mapEDUTypeSys.find(type) ;
   return it == mapEDUTypeSys.end()?false:true ;
}

pmdEDUCB::pmdEDUCB ( pmdEDUMgr *mgr, EDU_TYPES type ):
    _type(type),
    _mgr(mgr),
    _status(PMD_EDU_CREATING),
    _id(0),
    _isForced(false),
    _isDisconnected(false)
{
}

struct _eduEntryInfo
{
   EDU_TYPES     type ;
   int           regResult ;
   pmdEntryPoint entryFunc ;
};

#define ON_EDUTYPE_TO_ENTRY1(type, system, entry, desp) \
    {type, registerEDUName(type, desp, system), entry}

pmdEntryPoint getEntryFuncByType(EDU_TYPES type)
{
    pmdEntryPoint rt = NULL;
    static const _eduEntryInfo entry[] = {
        ON_EDUTYPE_TO_ENTRY1(EDU_TYPE_AGENT, false,
                            pmdAgentEntryPoint,
                            "Agent"),
        ON_EDUTYPE_TO_ENTRY1(EDU_TYPE_TCPLISTENER, true,
                            pmdTcpListenerEntryPoint,
                            "TCPListener"),
        ON_EDUTYPE_TO_ENTRY1(EDU_TYPE_MAXIMUM, false,
                            NULL,
                            "Unkown")
    };

    static const unsigned int number = sizeof(entry)/sizeof(_eduEntryInfo);

    unsigned int index = 0;
    for(;index < number; ++index)
    {
        if(entry[index].type == type)
        {
            rt = entry[index].entryFunc;
            goto done;
        }
    }

done:
    return rt;
error:
    goto done;
}

int pmdRecv(char *pBuffer, int recvSize, ossSocket *sock, pmdEDUCB *cb)
{
    int rc = EDB_OK;
    EDB_ASSERT(sock, "Soket is NULL");
    EDB_ASSERT(cb, "cb is NULL");
    while(true)
    {
        if(cb -> isForced())
        {
            rc = EDB_APP_FORCED;
            goto done;
        }
        rc = sock->recv(pBuffer, recvSize);
        if(EDB_TIMEOUT == rc)
        {
            continue;
        }
        goto done;
    }
done:
    return rc;
error:
    goto done;
}

int pmdSend(const char *pBuffer, int sendSize, ossSocket *sock, pmdEDUCB *cb)
{
    int rc = EDB_OK;
    EDB_ASSERT(sock, "Socket is NULL");
    EDB_ASSERT(cb, "cb is NULL");
    while(true)
    {
        if(cb->isForced())
        {
            rc = EDB_APP_FORCED;
            goto done;
        }
        rc = sock->send(pBuffer, sendSize);
        if(EDB_TIMEOUT == rc)
        {
            continue;
        }
        goto done;
    }

done:
    return rc;
error:
    goto done;
}

int pmdEDUEntryPoint(EDU_TYPES type, pmdEDUCB *cb, void *arg)
{
    int rc = EDB_OK;
    EDUID eduID = cb->getID();
    pmdEDUMgr *eduMgr = cb->getEDUMgr();
    pmdEDUEvent event;
    bool eduDestroyed = false;
    bool isForced = false;
    // main loop
    while(!eduDestroyed)
    {
        type = cb->getType();
        // wait for 1000 milliseconds for events
        if(!cb->waitEvent(event, 1000))
        {
            // 没有等待到Event
            if(cb->isForced())
            {   
                // EDU 被外部强行杀死
                PD_LOG(PDEVENT, "EDU %lld is forced", eduID);
                isForced = true;
            }
            else
            {
                // 继续等待Event
                continue;
            }
        }
        if(!isForced && PMD_EDU_EVENT_RESUME == event._eventType)
        {
            // set EDU status to wait
            eduMgr->waitEDU(eduID);
            // run the main function
            pmdEntryPoint entryFunc = getEntryFuncByType(type);
            if(!entryFunc)
            {
                PD_LOG(PDERROR, "EDU %lld type %d entry point func is NULL",
                    eduID, type);
                EDB_SHUTDOWN_DB
                rc = EDB_SYS;
            }
            else
            {
                rc = entryFunc(cb, event._Data);
            }
            // sanity check
            if(EDB_IS_DB_UP)
            {
                // system EDU should never exit when db is still runnig
                if(isSystemEDU(cb->getType()))
                {
                    PD_LOG(PDSEVERE, "System EDU: %lld, type %s, exits with %d",
                            eduID, getEDUName(type), rc);
                    EDB_SHUTDOWN_DB
                }
                else if(rc)
                {
                    PD_LOG(PDWARNING, "EDU %lld, type %s, exits with %d",
                            eduID, getEDUName(type), rc);
                }
            }
            eduMgr->waitEDU(eduID);
        }
        else if(!isForced && PMD_EDU_EVENT_TERM != event._eventType)
        {
            PD_LOG(PDERROR, "Receive the wrong event %d in EDU %lld, type %s",
                    event._eventType, eduID, getEDUName(type));
            rc = EDB_SYS;
        }
        else if(isForced && PMD_EDU_EVENT_TERM == event._eventType)
        {
            PD_LOG(PDEVENT, "EDU %lld, type %s is forced", eduID, type);
            isForced = true;
        }
        // release the event data
        if(!isForced && event._Data && event._release)
        {
            free(event._Data);
            event.reset();
        }
        rc = eduMgr->returnEDU(eduID, isForced, &eduDestroyed);
        if(rc)
        {
            PD_LOG(PDERROR, "Invalid EDU status for EDU: %lld, type %s",
                    eduID, getEDUName(type));
        }
        PD_LOG(PDDEBUG, "Terminating threads for EDU: %lld, type %s",
                    eduID, getEDUName(type));
    }
return 0;
}

