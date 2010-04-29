#include "llvm/Instruction.h"
#include "llvm/Analysis/DebugInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include <string>

#ifndef UTILS_H
#define UTILS_H

using namespace llvm;

const char* getSourceFile(Instruction* inst);
const char* getSourceFile(Loop* loop);
int getLineNumber(Instruction* inst);
int getLineNumber(Loop* loop);

#endif //UTILS_H
