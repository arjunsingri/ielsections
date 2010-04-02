//===-- ArrayDef.cpp - ArrayDef class code ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the defintion of the ArrayDef class, which is 
// used for computing control dependences.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Instructions.h"
#include "llvm/Analysis/PostDominators.h"
#include "ReachingDef.h"
#include <queue>
#include <list>
#include <iostream>

using namespace llvm;

void BasicBlockDup::addToKillSet(std::vector<Instruction*>& killed)
{
    m_killSet.insert(killed.begin(), killed.end());
}

char ReachingDef::ID;
static RegisterPass<ReachingDef> 
C("rd", "compute reaching definitions for structures and arrays");

//===----------------------------------------------------------------------===//
// ReachingDef Implementation
//===----------------------------------------------------------------------===//

void ReachingDef::clear(void)
{
    m_assignmentMap.clear();
    m_killedMap.clear();
    m_udChain.clear();

    for (BasicBlockDupMapType::iterator i = m_basicBlockDupMap.begin(); i != m_basicBlockDupMap.end(); ++i)
    {
        free((*i).second);
    }

    m_basicBlockDupMap.clear();
}

ReachingDef::~ReachingDef(void)
{
    for (BasicBlockDupMapType::iterator i = m_basicBlockDupMap.begin(); i != m_basicBlockDupMap.end(); ++i)
    {
        free((*i).second);
    }
}

//pointerOperand would be the operand for either a store inst or a load inst
Value* ReachingDef::findCoreOperand(Value* pointerOperand)
{
    Value* coreOperand = NULL;

    if (GetElementPtrInst* getElementPtrInst = dyn_cast<GetElementPtrInst>(pointerOperand))
    {
        GetElementPtrInst* operand = getElementPtrInst;
        GetElementPtrInst* previous;

        do 
        {
            previous = operand;
            operand = dyn_cast<GetElementPtrInst>(operand->getPointerOperand());
        } while (operand != NULL);

        getElementPtrInst = previous;

        const Type* elementType = getElementPtrInst->getPointerOperandType()->getElementType();

        if (elementType->getTypeID() == Type::StructTyID)
        {
            coreOperand = getElementPtrInst->getPointerOperand();
        }
        else if (elementType->getTypeID() == Type::ArrayTyID)
        {
            coreOperand = getElementPtrInst->getPointerOperand();
        }
    }
    
    return coreOperand;
}

void ReachingDef::findDownwardsExposed(BasicBlock* block)
{
    BasicBlockDup* currentDup = new BasicBlockDup(block);
    m_basicBlockDupMap.insert(BasicBlockDupElementType(block, currentDup));
 
    for (BasicBlock::iterator i = block->begin(); i != block->end(); ++i)
    {
        Instruction& inst = *i;

        if (StoreInst* storeInst = dyn_cast<StoreInst>(&inst))
        {
            Value* coreOperand = findCoreOperand(storeInst->getPointerOperand());

            AssignmentMapType::iterator where = m_assignmentMap.find(coreOperand);
            if (where != m_assignmentMap.end())
            {
                std::vector<Instruction*>& killed = m_assignmentMap[coreOperand];
                for (std::vector<Instruction*>::iterator j = killed.begin(); j != killed.end(); ++j)
                {
                    //TODO: killed map can be maintained as a vector of sets, each value in a set kills all other values
                    //search becomes harder though
                    m_killedMap[*j].push_back(storeInst);
                    m_killedMap[storeInst].push_back(*j);
                }
            }

            m_assignmentMap[coreOperand].push_back(storeInst);
 
            LastWriteMapType& currentLastWriteMap = currentDup->getLastWriteMap();
            DownwardsExposedMapType& currentDownwardsExposedMap = currentDup->getDownwardsExposedMap();

            LastWriteMapType::iterator where2 = currentLastWriteMap.find(coreOperand);
            if (where2 != currentLastWriteMap.end())
            {
                //set the last instruction that was in the last write map to be not downwards exposed
                currentDownwardsExposedMap[currentLastWriteMap[coreOperand]] = false;
            }

            currentLastWriteMap[coreOperand] = storeInst;
            currentDownwardsExposedMap[storeInst] = true; //downwards exposed until we find the next write for coreOperand
        }
    }
}

void ReachingDef::constructGenSet(BasicBlock* block)
{
    BasicBlockDup* basicBlockDup = m_basicBlockDupMap[block];
    assert(basicBlockDup != NULL && "no match for a duplicate basic block!");

    DownwardsExposedMapType& downwardsExposedMap = basicBlockDup->getDownwardsExposedMap();
    
    for (BasicBlock::iterator i = block->begin(); i != block->end(); ++i)
    {
        Instruction& inst = *i;
        DownwardsExposedMapType::iterator where = downwardsExposedMap.find(&inst);

        if (where != downwardsExposedMap.end())
        {
            if (where->second)
            {
                basicBlockDup->addToGenSet(&inst);
            }
        }
    }

    std::cout << basicBlockDup->getGenSet().size() << std::endl;
}

void ReachingDef::constructKillSet(BasicBlock* block)
{
    BasicBlockDup* basicBlockDup = m_basicBlockDupMap[block];
    assert(basicBlockDup != NULL && "no match for a duplicate basic block!");

    for (BasicBlock::iterator i = block->begin(); i != block->end(); ++i)
    {
        Instruction& inst = *i;

        KilledMapType::iterator where = m_killedMap.find(&inst);
        if (where != m_killedMap.end())
        {
            basicBlockDup->addToKillSet(where->second);
        }
    }
}

void ReachingDef::constructInSets(Function& function)
{
    std::queue<BasicBlock*> worklist;
    std::map<BasicBlock*, bool> visited;

    // do a BFS and find all CFG edges (C, B) such that B does not post-dominate C

    //FIXME: is there a way to simply traverse all cfg edges instead of doing a BFS?
    worklist.push(&function.getEntryBlock());
    while (!worklist.empty())
    {
        BasicBlock* block = worklist.front();
        worklist.pop();

        BasicBlockDup* dup = m_basicBlockDupMap[block];
        assert(dup != NULL);

        InSetType& inSet = dup->getInSet();
        KillSetType& killSet = dup->getKillSet();
        InSetType tempSet;

        std::set_difference(inSet.begin(), inSet.end(), killSet.begin(), killSet.end(), inserter(tempSet, tempSet.begin()));
        std::cout << "tempSet: " << tempSet.size() << std::endl;

        GenSetType& genSet = dup->getGenSet();
        OutSetType& outSet = dup->getOutSet();
        std::set_union(genSet.begin(), genSet.end(), tempSet.begin(), tempSet.end(), inserter(outSet, outSet.begin()));
   
        for (succ_iterator succ = succ_begin(block); succ != succ_end(block); ++succ)
        {
            BasicBlockDup* succDup = m_basicBlockDupMap[*succ];
            InSetType& succInSet = succDup->getInSet();
            unsigned int oldInSize = succInSet.size();
    
            succInSet.insert(outSet.begin(), outSet.end());
            if (succInSet.size() != oldInSize)
            {
                worklist.push(*succ);
            }
        }
    }
}

void ReachingDef::findDefinitions(Value* coreOperand, BasicBlockDup* blockDup, Instruction* inst)
{
    InSetType& inSet = blockDup->getInSet();
    std::cout << "inset size: " << inSet.size() << std::endl;
    for (InSetType::iterator i = inSet.begin(); i != inSet.end(); ++i)
    {
        StoreInst *storeInst = dyn_cast<StoreInst>(*i);
        assert(storeInst != NULL && "not a store instruction!");
        if (coreOperand == findCoreOperand(storeInst->getPointerOperand()))
        {
            m_udChain[inst].push_back(storeInst);
        }
    }
}

void ReachingDef::constructUDChain(Function& function)
{
    for (inst_iterator i = inst_begin(function); i != inst_end(function); ++i)
    {
        Instruction& inst = *i;
        if (LoadInst *loadInst = dyn_cast<LoadInst>(&inst))
        {
            BasicBlockDup* blockDup = m_basicBlockDupMap[inst.getParent()];
            
            Value* coreOperand = findCoreOperand(loadInst->getPointerOperand());
            findDefinitions(coreOperand, blockDup, &inst);
        }
    }
}

// Compute reaching definitions for arrays in all basic blocks of this function
//
bool ReachingDef::runOnFunction(Function& function)
{
    clear();
    std::cout << "starting!\n";
    for (Function::iterator i = function.begin(); i != function.end(); ++i)
    {
        BasicBlock& block = *i;
        
        findDownwardsExposed(&block);
        constructGenSet(&block);
        constructKillSet(&block);
    }

    constructInSets(function);
    constructUDChain(function);
    std::cout << "size " << m_udChain.size() << std::endl;
    printa();

    return false;
}

void ReachingDef::printa(void)
{
    for (UDChainMapType::iterator i = m_udChain.begin(); i != m_udChain.end(); ++i)
    {
        std::cout << "For " << *i->first << std::endl;
        std::vector<Instruction*>& rd = i->second;
        for (std::vector<Instruction*>::iterator j = rd.begin(); j != rd.end(); ++j)
        {
            std::cout << "\t" << *j << std::endl;
        }
    }
}

void ReachingDef::getAnalysisUsage(AnalysisUsage& AU) const
{
    AU.setPreservesAll();
}

