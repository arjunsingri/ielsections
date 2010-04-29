#include "utils.h"

int getLineNumber(Loop* loop)
{
    return getLineNumber(loop->getHeader()->getFirstNonPHI());
}

int getLineNumber(Instruction* inst)
{
    if (MDNode* node = inst->getMetadata("dbg"))
    {
        DILocation loc(node);
        return loc.getLineNumber();
    }

    assert(false);
    return -1;
}

const char* getSourceFile(Loop* loop)
{
    return getSourceFile(loop->getHeader()->getFirstNonPHI());
}

const char* getSourceFile(Instruction* inst)
{
    if (MDNode* node = inst->getMetadata("dbg"))
    {
        DILocation loc(node);
        return loc.getFilename().str().c_str();
    }

    assert(false);

    return "";
}
