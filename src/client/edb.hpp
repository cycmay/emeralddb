#ifndef EDB_HPP__
#define EDB_HPP__

#include "core.hpp"
#include "ossSocket.hpp"
#include "commandFactory.hpp"
const int CMD_BUFFER_SIZE = 512;

class Edb
{
    public:
        Edb() {};
        ~Edb() {};
    
    public:
        void start(void);
    
    protected:
        void prompt(void);
    
    private:
        void        split(const std::string &text, char delim, std::vector<std::string> &result);
        char*       readLine(char *p, int length);
        int         readInput(const char* pPrompt, int numIndent);
    
    private:
        ossSocket       _sock;
        CommandFactory  _cmdFactory;
        char            _cmdBuffer[CMD_BUFFER_SIZE];
};

#endif