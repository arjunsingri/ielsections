//===- ControlDependence.h - ControlDependence class definition -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the ControlDependence class, which is
// used for computing control dependences.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CONTROLDEPENDENCE_H
#define LLVM_CONTROLDEPENDENCE_H

#include <vector>
#include <map>
#include <list>
#include <queue>

namespace llvm {

//===----------------------------------------------------------------------===//
//
// ControlDependence class - This class computes the control dependences 
//    among basic blocks for a function
//    A node n is control dependent on a node c if
// 1) there exists an edge e1 coming out of c that definitely causes n to
//    execute
// 2) there exists some edge e2 coming out of c that is the start of some path
//    that avoids the execution of n
//
class ControlDependence : public FunctionPass {
    // type for representing control dependents - if a is control dependent on b
    // then a will be in the vector for b
    //
    typedef std::map<BasicBlock*, std::vector<BasicBlock*> > ControlDependentTy;
    ControlDependentTy ControlDependents;

    // type for representing control dependency - if a is control dependent on b 
    // then b will be in the vector for a
    //
    typedef std::map<BasicBlock*, std::vector<BasicBlock*> > ControlDependenceTy;
    ControlDependenceTy ControlDependences;

    public:
        static char ID;
        ControlDependence() : FunctionPass(&ID) {}

        //return true if B is control dependent on A
        bool isControlDependent(BasicBlock* B, BasicBlock* A) const;

        //return true if instruction B is control dependent on instruction A
        bool isControlDependent(Instruction* B, Instruction* A) const;

        // return all the basic blocks on which B is control dependent
        // TODO:B should actually belong to the current function but there is no way
        // to enforce that without holding a reference to the function inside
        // the control dependence class
        //
        std::vector<BasicBlock*> getControlDependences(BasicBlock* B) {
            assert(B != NULL && "Input block cannot be NULL");
            return ControlDependences[B];
        }

        //iterator for traversing the blocks on which P is control dependent
        std::vector<BasicBlock*>::const_iterator dependence_begin(BasicBlock* P) { return ControlDependences[P].begin(); }

        std::vector<BasicBlock*>::const_iterator dependence_end(BasicBlock* P) { return ControlDependences[P].end(); }

        // return the compare instructions on which instruction A is control dependent. This is basically the list
        // of all compare instructions in all the basic blocks on which the containing block of A is
        // control dependent
        //
        std::vector<Instruction*> getControlDependenceInstructions(Instruction* A);

        // Compute control dependences for all blocks in this function
        virtual bool runOnFunction(Function& F);

        virtual void getAnalysisUsage(AnalysisUsage& AU) const;

        //print - Show contents in human readable format...
        virtual void print(std::ostream& O, const Module* = 0) const;

    private:
        
        Instruction* getBranchCondition(BasicBlock* B) const;
};

} //End llvm namespace

#endif // LLVM_CONTROLDEPENDENCE_H

