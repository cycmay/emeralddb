#include "core.hpp"
#include "pmdEDU.hpp"
#include "ossSocket.hpp"
#include "pmd.hpp"
#include "pmdEDUMgr.hpp"
#include "pd.hpp"

#define PMD_TCPLISTENER_RETRY 5
#define OSS_MAX_SERVICENAME NI_MAXSERV

int pmdTcpListenerEntryPoint(pmdEDUCB *cb, void *arg)
{
    int rc                  = EDB_OK;
    pmdEDUMgr * eduMgr      = cb->getEDUMgr();
    EDUID eduID             = cb->getID();
    unsigned int retry      = 0;
    EDUID agentEDU          = PMD_INVALID_EDUID;
    char                    svcName[OSS_MAX_SERVICENAME + 1];

    while(retry <= PMD_TCPLISTENER_RETRY && !EDB_IS_DB_DOWN)
    {
        retry++;

        strcpy(svcName, pmdGetKRCB()->getSvcName() );
        PD_LOG(PDEVENT, "Listening on port_test: %s\n", svcName);

        int port = 0;
        int len = strlen(svcName);
        for (int i = 0; i < len; ++i)
        {
            if(svcName[i] >= '0' && svcName[i] <= '9')
            {
                port = port*10;
                port += int(svcName[i]-'0');
            }
            else
            {
                PD_LOG(PDERROR, "service name error!");
            }
        }

        ossSocket sock(port);
        rc = sock.initSocket();
        EDB_VALIDATE_GOTOERROR(EDB_OK==rc, rc, "Failed initialize socket");

        rc = sock.bind_listen();
        EDB_VALIDATE_GOTOERROR(EDB_OK==rc, rc, "Failed to bind_listen socket");

        // once bind is successful, set the state of EDU to RUNNING
        if(EDB_OK != (rc = eduMgr->activateEDU(eduID)) )
        {
            goto error;
        }
        // master loop for tcp listener
        while(!EDB_IS_DB_DOWN)
        {
            int s;
            // accept() default timeout=10ms, 
            rc = sock.accept(&s, NULL, NULL);
            // get nothing for a period of time, loop
            if(EDB_TIMEOUT == rc)
            {
                rc = EDB_OK;
                continue;
            }
            // receive error due to database down
            if(rc && EDB_IS_DB_DOWN)
            {
                rc = EDB_OK;
                goto done;
            }
            else if(rc)
            {
                // error, restart socket
                PD_LOG(PDERROR, "Failed to accept socket in TCP listener");
                PD_LOG(PDEVENT, "Restarting socket to listen");
                break;
            }

            void *pData = NULL;
            *((int *) &pData) = s;

            // 启动代理线程 用于处理客户端发送的请求
            rc = eduMgr->startEDU(EDU_TYPE_AGENT, pData, &agentEDU);
            if(rc)
            {
                if(rc == EDB_QUIESCED)
                {
                    // we cannot start EDU due to quiesced
                    PD_LOG(PDWARNING, "reject new connection due to quiesced database");

                }
                else
                {
                    PD_LOG(PDERROR, "Failed to start EDU agent");
                }
                // close remote connection if we cannot create new thread
                ossSocket newsock(&s);
                newsock.close();
                continue;
            }
        }
        if(EDB_OK != (rc = eduMgr->waitEDU(eduID)) )
        {
            goto error;
        }
    }   // while(retry <= PMD_TCPLISTENER_RETRY)

done:
    return rc;
error:
    switch(rc)
    {
        case EDB_SYS:
            PD_LOG(PDSEVERE, "System error occured");
            break;
        default:
            PD_LOG(PDSEVERE, "Internal error");
    }
    goto done;
}

