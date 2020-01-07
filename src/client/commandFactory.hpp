#ifndef COMMANDFACTORY_HPP__
#define COMMANDFACTORY_HPP__

#include "command.hpp"                                                     \

#define COMMAND_BEGIN void CommandFactory::addCommand() {
#define COMMAND_END }
#define COMMAND_ADD(cmdName,cmdClass) {                         \
   ICommand* pObj = new cmdClass();                             \
   std::string str = cmdName;                                   \
   _cmdMap.insert(COMMAND_MAP::value_type(str,pObj));           \
   } 

class CommandFactory
{
    typedef std::map<std::string, ICommand*> COMMAND_MAP;
    public:
        CommandFactory();
        ~CommandFactory(){}
        void addCommand();
        ICommand * getCommandProcesser(const char * pcmd);
    private:
        COMMAND_MAP _cmdMap;
};
#endif