//===-- ControlDependence.cpp - ControlDependence class code ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the defintion of the ControlDependence class, which is 
// used for computing control dependences.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/PostDominators.h"
#include "ControlDependence.h"
#include <iostream>

using namespace llvm;

char ControlDependence::ID;
static RegisterPass<ControlDependence> 
C("control-dependence", "Compute control dependences");

//===----------------------------------------------------------------------===//
// ControlDependence Implementation
//===----------------------------------------------------------------------===//

//isControlDependence - return true if B is control dependent on A
//
bool ControlDependence::isControlDependent(BasicBlock* B, BasicBlock* A) const {
    assert(A != NULL && B != NULL && "Inputs cannot be NULL");

    // find all the blocks that are control dependent on A
    ControlDependentTy::const_iterator where = ControlDependents.find(A);
    if (where != ControlDependents.end())
    {
        // check to see if B is one of those blocks
        std::vector<BasicBlock*>::const_iterator where2 = 
            std::find(where->second.begin(), where->second.end(), B);
        if (where2 != where->second.end())
        {
            return true;
        }
    }

    return false;
}

// Return true if B is control dependent on A. For this to work,
// A must be the branch instruction in its basic block
//
bool ControlDependence::isControlDependent(Instruction* B, Instruction* A) const {
    assert(A != NULL && B != NULL && "Inputs cannot be NULL");

    if (A != getBranchInstruction(A->getParent()))
    {
        return false;
    }

    return isControlDependent(B->getParent(), A->getParent());
}

// Return the compare instructions on which instruction A is control dependent.
// This is basically the list of all compare instructions used for the branch 
// in all the basic blocks on which the containing block of A is 
// control dependent
//
std::vector<Instruction*> 
ControlDependence::getControlDependenceInstructions(Instruction* A) {
    std::vector<Instruction*> DepInsts;

    BasicBlock* P = A->getParent();
    for (std::vector<BasicBlock*>::const_iterator I = dependence_begin(P), E = dependence_end(P);
            I != E; ++I)
    {
        Instruction* B = getBranchInstruction(*I);
        DepInsts.push_back(B);
    }

    return DepInsts;
}


// Compute control dependences for all basic blocks of this function
//
bool ControlDependence::runOnFunction(Function& F) {
    ControlDependents.clear();
    ControlDependences.clear();

    PostDominatorTree& PDT = getAnalysis<PostDominatorTree>(); 

    typedef std::pair<BasicBlock*, BasicBlock*> CFGEdge;
    std::list<CFGEdge> notDominated;

    DenseMap<BasicBlock*, bool> Visited;
    std::queue<BasicBlock*> BQ;

    // do a BFS and find all CFG edges (C, B) such that B does not post-dominate C
 
    //FIXME: is there a way to simply traverse all cfg edges instead of doing a BFS?
    BQ.push(&F.getEntryBlock());
    while (!BQ.empty())
    {
        BasicBlock* C = BQ.front();
        BQ.pop();

        for (succ_iterator I = succ_begin(C), E = succ_end(C); I != E; ++I)
        {
            BasicBlock* B = *I;

            if (!PDT.dominates(B, C))
            {
                notDominated.push_back(CFGEdge(C, B));
                assert(getBranchInstruction(C) != NULL && "bug in PDT!");
            }

            if (Visited.count(B) == false)
            {
                //mark block C as visited
                Visited.insert(std::pair<BasicBlock*, bool>(B, true));
                BQ.push(B);
            }
        }
    }

    
    // For every CFG edge (C, B) that satisfied the conditions above, traverse
    // the post-dominator tree bottom up starting from B and stopping just 
    // before the parent of C. For every N node visited in this tree, store
    // N as being control dependent on C
    //
    for (std::list<CFGEdge>::iterator I = notDominated.begin(),
            E = notDominated.end(); I != E; ++I)
    {
        BasicBlock* CbasicBlock = I->first;
        DomTreeNode* CdomTreeNode = PDT[CbasicBlock];

        DomTreeNode* BdomTreeNode = PDT[I->second];
        DomTreeNode* NdomTreeNode = BdomTreeNode;
        BasicBlock* NbasicBlock = NdomTreeNode->getBlock();

        do
        {
            ControlDependents[CbasicBlock].push_back(NbasicBlock);

            ControlDependences[NbasicBlock].push_back(CbasicBlock);

            NdomTreeNode = NdomTreeNode->getIDom();

        } while (NdomTreeNode != NULL && NdomTreeNode != CdomTreeNode->getIDom());
    }

    return false;
}

Instruction* ControlDependence::getBranchInstruction(BasicBlock* B) const {
    assert(B != NULL && "Input cannot be NULL");

/*
    if (ControlDependents.find(B) == ControlDependents.end())
    {
        std::cout << B->getName().str() << " has not control dependents\n";
        return NULL;
    }
*/
    TerminatorInst* TI = B->getTerminator();
    assert(TI != NULL && "Incorrect control dependence");

    Instruction* C = NULL;
    if (BranchInst* BI = dyn_cast<BranchInst>(TI))
    {
        assert(BI->isConditional() && "Incorrect control dependence");
        C = BI; 
    }
    else if (SwitchInst* SI = dyn_cast<SwitchInst>(TI))
    {
        C = SI;
    }
    else if (InvokeInst *II = dyn_cast<InvokeInst>(TI))
    {
        C = II;
    }

/*
    if (C ==  NULL)
    {
        TI->dump();
        std::cout << B->getName().str() << " of " << B->getParent()->getName().str() << std::endl;
    }
*/
    return C;
}

void ControlDependence::getAnalysisUsage(AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AU.addRequired<PostDominatorTree>();
}

//print - Show contents in human readable format...
void ControlDependence::printDependences(std::ostream& O) const {
    for (ControlDependentTy::const_iterator I = ControlDependents.begin(), 
            E = ControlDependents.end(); I != E; ++I)
    {
        const std::vector<BasicBlock*>& dependents = I->second;
        for (std::vector<BasicBlock*>::const_iterator J = dependents.begin(),
                                                F = dependents.end(); J != F; ++J)
        {
            O << (*J)->getName().str() << " --> " << I->first->getName().str() << "\n";
        }
    }
}

