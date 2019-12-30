#ifndef OSSPRIMITIVEFILEOP_HPP__
#define OSSPRIMITIVEFILEOP_HPP__

#include "core.hpp"

// 定义一些标准Linux操作宏
#define OSS_F_GETLK             F_GETLK64
#define OSS_F_SETLK             F_SETLK64
#define OSS_F_SETLKW            F_SETLKW64

#define oss_struct_statfs       struct statfs64
#define oss_statfs              statfs64
#define oss_fstatfs             fstatfs64
#define oss_struct_statvfs      struct statvfs64
#define oss_statvfs             statvfs64
#define oss_struct_stat         struct stat64
#define oss_struct_flock        struct flock64
#define oss_stat                stat64
#define oss_lstat               lstat64
#define oss_fstat               fstat64
#define oss_open                open64
#define oss_lseek               lseek64
#define oss_ftruncate           ftruncate64
#define oss_off_t               off64_t
#define oss_close               close
#define oss_access              access 
#define oss_chmod               chmod
#define oss_read                read
#define oss_write               write

#define OSS_PRIMITIVE_FILE_OP_FWRITE_BUF_SIZE   2048
#define OSS_PRIMITIVE_FILE_OP_READ_ONLY         (((unsigned int)1) << 1)
#define OSS_PRIMITIVE_FILE_OP_WRITE_ONLY        (((unsigned int)1) << 2)
#define OSS_PRIMITIVE_FILE_OP_OPEN_EXISTING     (((unsigned int)1) << 3)
#define OSS_PRIMITIVE_FILE_OP_OPEN_ALWAYS       (((unsigned int)1) << 4)
#define OSS_PRIMITIVE_FILE_OP_OPEN_TRUNC        (((unsigned int)1) << 5)

#define OSS_INVALID_HANDLE_FD_VALUE (-1)

typedef oss_off_t offsetType;

class ossPrimitiveFileOp
{
    public:
        typedef int handleType;
    private:
        handleType  _fileHandle;
        // 构造方法的两种方式 禁止从一个对象生成另外一个对象
        ossPrimitiveFileOp(const ossPrimitiveFileOp &){};
        const ossPrimitiveFileOp &operator=(const ossPrimitiveFileOp &);
        bool _bIsStdout;

    protected:
        void setFileHandle(handleType handle);

    public:
        ossPrimitiveFileOp();
        int Open(
            const char * pFilePath, 
            unsigned int options = OSS_PRIMITIVE_FILE_OP_OPEN_ALWAYS
        ); 
        void openStdout();
        void Close();
        bool isValid(void);
        int Read(const size_t size, void * const pBuf, int * const pBytesRead);
        int Write(const void *pBuf, size_t len=0);
        int fWrite(const char *fmt, ...);
        offsetType getCurrentOffset(void) const;
        void seekToOffset(offsetType offset);
        void seekToEnd(void);
        int getSize(offsetType * const pFileSize);

};

#endif