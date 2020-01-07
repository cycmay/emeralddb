#include "commandFactory.hpp"

CommandFactory::CommandFactory()
{
    addCommand();
}

ICommand *CommandFactory::getCommandProcesser(const char* pcmd)
{
    ICommand * pProcessor = NULL;
    do{
        COMMAND_MAP::iterator iter;
        iter = _cmdMap.find(pcmd);
        if(iter != _cmdMap.end())
        {
            pProcessor = iter->second;
        }
    }while(0);
    return pProcessor;
}