#include "core.hpp"
#include "pd.hpp"
// 加入锁 保证日志的线程安全性
#include "ossLatch.hpp"

#include "ossPrimitiveFileOp.hpp"

// 对日志级别定义字符串
const static char *PDLEVELSTRING[] = {
    "SEVERE",
    "ERROR",
    "EVENT",
    "WARNING",
    "INFO",
    "DEBUG"
};
const char * getPDLevelDesp(PDLEVEL level)
{
    if((unsigned int)level > (unsigned int)PDDEBUG)
    {
        return "Unkown Level";
    }
    return PDLEVELSTRING[(unsigned int)level];

}

const static char *PD_LOG_HEADER_FORMAT="%04d-%02d-%02d-%02d:%02d:%02d:%06d\
Level:%s" OSS_NEWLINE"PID:%-37dTID:%d" OSS_NEWLINE"Funtion:%-32sLine:%d"\
 OSS_NEWLINE"File:%s" OSS_NEWLINE"Message:" OSS_NEWLINE"%s" OSS_NEWLINE OSS_NEWLINE;

PDLEVEL _curPDLevel = PD_DFT_DIAGLEVEL;
char _pdDiagLogPath [OSS_MAX_PATHSIZE+1] = {0};

ossXLatch _pdLogMutex;
ossPrimitiveFileOp _pdLogFile;

// open log file
static int _pdLogFileReopen()
{
    int rc = EDB_OK;
    _pdLogFile.Close();
    rc = _pdLogFile.Open(_pdDiagLogPath);
    if(rc)
    {
        printf("Failed to open log file, errno = %d" OSS_NEWLINE, rc);
        goto error;
    }
    _pdLogFile.seekToEnd();
done:
    return rc;
error:
    goto done;
}

// write log file
static int _pdLogFileWrite(const char *pData)
{   
    int rc = EDB_OK;
    size_t dataSize = strlen(pData);
    _pdLogMutex.get();
    if(!_pdLogFile.isValid())
    {
        // open the file
        rc = _pdLogFileReopen();
        if(rc)
        {
            printf("Failed to open log file, error = %d" OSS_NEWLINE, rc);
            goto error;
        }
    }
    // write to the file
    rc = _pdLogFile.Write(pData, dataSize);
    if(rc) 
    {
        printf("Failed to write into log file, error = %d" OSS_NEWLINE, rc);
        goto error;
    }

done:
    _pdLogMutex.release();
    return rc;
error:
    goto done;
}

// log
void pdLog(PDLEVEL level, const char *func, const char *file, unsigned int line,
            const char *format, ...)
{
    int rc = EDB_OK;
    if(_curPDLevel < level)
        return;
    va_list ap;
    char userInfo[PD_LOG_STRINGMAX];    // for user define message
    char sysInfo[PD_LOG_STRINGMAX];     // for log header
    struct tm otm;
    struct timeval tv;
    struct timezone tz;
    time_t tt;

    gettimeofday(&tv, &tz);
    tt = tv.tv_sec;
    localtime_r (&tt, &otm);

    // create user information
    va_start(ap, format);
    vsnprintf(userInfo, PD_LOG_STRINGMAX, format, ap);
    va_end(ap);

    snprintf(sysInfo, PD_LOG_STRINGMAX, PD_LOG_HEADER_FORMAT,
            otm.tm_year+1900,
            otm.tm_mon+1,
            otm.tm_mday,
            otm.tm_hour,
            otm.tm_min,
            otm.tm_sec,
            tv.tv_usec,
            PDLEVELSTRING[level],
            getpid(),
            syscall(SYS_gettid),
            func,
            line,
            file,
            userInfo
    );

    printf("%s" OSS_NEWLINE, sysInfo);
    if(_pdDiagLogPath[0] != '\0')
    {
        rc = _pdLogFileWrite(sysInfo);
        if(rc)
        {
            printf("Failed to write into log file, error = %d" OSS_NEWLINE, rc);
            printf("%s" OSS_NEWLINE, sysInfo);
        }
    }
    return;
}