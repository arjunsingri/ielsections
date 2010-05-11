#include "utils.h"
#include <iostream>

//pointerOperand would be the operand for either a store inst or a load inst
std::set<Value*> findCoreOperand(Value* pointerOperand, Value** coreOperand, const Type** coreOperandType)
{
    assert(pointerOperand != NULL && "target of store inst obtained incorrectly");

    std::set<Value*> indices;
    if (isa<GetElementPtrInst>(pointerOperand) || isa<CastInst>(pointerOperand))
    {
        Value* operand = pointerOperand;
        Value* previous;
        bool more;

        do
        {
            more = false;
            if (GetElementPtrInst* getElementPtrInst = dyn_cast<GetElementPtrInst>(operand))
            {
                for (User::op_iterator index = getElementPtrInst->idx_begin(); index != getElementPtrInst->idx_end(); ++index)
                {
                    indices.insert(*index);
                }
                
                more = true;
                previous = operand;
                operand = getElementPtrInst->getPointerOperand();
            }
            else if (CastInst* castInst = dyn_cast<CastInst>(operand))
            {
                int j = 0;
                Value* op = NULL;
                for (User::op_iterator i = castInst->op_begin(); i != castInst->op_end(); ++i)
                {
                    op = *i;
                    ++j;
                }

                assert(j == 1);
                more = true;
                previous = operand;
                operand = op;
            }
        } while (more);

        //get - operand

        assert(operand != NULL);
        const PointerType* pointerType = dyn_cast<PointerType>(operand->getType());
        
        if (pointerType == NULL)
        {
            *coreOperand = NULL;
        }
        else
        {
            *coreOperand = operand;
            if (coreOperandType != NULL)
            {
                *coreOperandType = pointerType->getElementType();
            }
        }
    }
    else
    {
        *coreOperand = pointerOperand;
        if (coreOperandType != NULL)
        {
            *coreOperandType = pointerOperand->getType();
        }

        switch (pointerOperand->getType()->getTypeID())
        {
            case Type::IntegerTyID:
             //   std::cout << "integer\n";
                break;

            case Type::PointerTyID:
             //   std::cout << "pointer\n";
                break;

            default:
                std::cout << "unhandled\n";
                assert(false);
        }
    }

    return indices;
}

std::pair<int, int> getLineNumber(Loop* loop)
{
    int min = INT_MAX;
    int max = INT_MIN;

    for (LoopBase<BasicBlock, Loop>::block_iterator block = loop->block_begin(); block != loop->block_end(); ++block)
    {
        for (BasicBlock::iterator instr = (*block)->begin(); instr != (*block)->end(); ++instr)
        {
            int line = getLineNumber(instr);
            if (line != -1)
            {
                if (min > line)
                {
                    min = line;
                }

                if (max < line)
                {
                    max = line;
                }
            }
        }
    }

    return std::pair<int, int>(min, max);

    //return getLineNumber(loop->getHeader()->getFirstNonPHI());
}

int getLineNumber(Instruction* inst)
{
    if (MDNode* node = inst->getMetadata("dbg"))
    {
        DILocation loc(node);
        return loc.getLineNumber();
    }

    //assert(false);
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

    //assert(false);

    return "";
}
