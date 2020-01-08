#include "core.hpp"
#include "ossSocket.hpp"

int pmdTcpListenerEntryPoint()
{
    int rc = EDB_OK;
    int port = 12551;
    ossSocket sock(port);
    rc = sock.initSocket();
    if(rc)
    {
        printf("Failed to initialize socket, rc = %d", rc);
        goto error;
    }
    rc = sock.bind_listen();
    if(rc)
    {
        printf("Failed to bid/listen socket, rc = %d", rc);
        goto error;
    }
    // master loop for tcp listener
    while(true)
    {
        int s;
        rc = sock.accept(&s, NULL, NULL);
        // if we dont get anything from period of time, lets loop
        if(EDB_TIMEOUT == rc)
        {
            rc = EDB_OK;
            continue;
        }
        char buffer[1024];
        int size;
        ossSocket sock1(&s);
        sock1.disableNagle();
        do
        {
            rc = sock1.recv((char*)&size, 4);
            if(rc && rc!=EDB_TIMEOUT)
            {
                printf("Failed to receive size, rc = %d", rc);
                goto error;
            }
        }while(EDB_TIMEOUT == rc);
        do
        {
            rc = sock1.recv(&buffer[0], size-sizeof(int));
            if(rc && rc!=EDB_TIMEOUT)
            {
                printf("Failed to receive buffer, rc = %d", rc);
                goto error;
            }
        }while(EDB_TIMEOUT == rc);
        printf("%s\n", buffer);
        sock1.close();
    }

error:
    switch (rc)
    {
    case EDB_SYS:
        printf("system error occured");
        break;
    
    default:
        printf("internal error");
    }
    goto done;
done:
    return rc;
}