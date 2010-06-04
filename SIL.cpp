#include "SIL.h"
#include "llvm/Support/CommandLine.h"
#include "utils.h"

using namespace llvm;

cl::opt<std::string> graphFile("iel:graph", cl::desc("Specify filename for the dot file"), cl::value_desc("filename"));
cl::opt<bool> printRejected("iel:print-rejected", cl::desc("Print rejected candidates for IE/L-sections"));
cl::opt<bool> printCount("iel:print-counts", cl::desc("Print the number of IE/L-sections and IE/L-section candidates"));
cl::opt<bool> outerLoops("iel:outer-loops", cl::desc("Check outer loops first"));
cl::opt<bool> skipEmptyBodyLoops("iel:skip-empty-body-loops", cl::desc("Skill all loops with empty bodies"));

char SIL::ID = 0;
static RegisterPass<SIL> sil("iel", "find all IE/L sections");

SIL::SIL()
    :   //LoopPass(&ID),
        FunctionPass(&ID),
        m_currentReachingDef(NULL),
        m_id(0)
{
//    std::string filename = "/home/singri/llvm-2.7/llvm/lib/Analysis/ielsections/Untitled1";
    std::string filename = graphFile.c_str();
    m_file.open(filename.c_str(), std::fstream::out);
    m_file << "digraph{\n";
}

SIL::~SIL()
{
    m_file << "\n}";
    m_file.close();
}

//perform step1 as per the paper and at the same time construct set RD for each triplet
void SIL::runStep1(IELSection* ielSection)
{
    assert(ielSection != NULL);
    SILParameterList& silParameters = ielSection->getSILParameters();

    for (SILParameterList::iterator i = silParameters.begin(); i != silParameters.end(); ++i)
    {
        SILParameter* currentParameter = *i;
        currentParameter->constructDefinitionList(m_currentReachingDef);
       
        bool isInside = false, isOutside = false;
        Loop* loop = ielSection->getLoop();

        for (unsigned int j = 0; j < currentParameter->getNumDefinitions(); ++j)
        {
            BasicBlock* parent = currentParameter->getDefinitionParent(j);

            if (loop->contains(parent) && loop->getHeader() != parent)
            {
                currentParameter->addRD(j);
                isInside = true;
            }
            else
            {
                isOutside = true;
            }
        }

        if (isInside && isOutside)
        {
            currentParameter->setSILValue(False, SILParameter::Step1);
        }
        else
        {
            currentParameter->setSILValue(DontKnow);
        }
    }
}

void SIL::computeCP(IELSection* ielSection, SILParameter* silParameter, ControlDependence& cd)
{
    assert(silParameter->getLoop() == ielSection->getLoop());

    Loop* loop = silParameter->getLoop();
    const std::vector<Instruction*>& deps = cd.getControlDependenceInstructions(silParameter->getInstruction());

    for (std::vector<Instruction*>::const_iterator j = deps.begin(); j != deps.end(); ++j)
    {
        BasicBlock* instructionParent = (*j)->getParent();

        if (loop->contains(instructionParent) && loop->getHeader() != instructionParent)
        {
            silParameter->addCP(*j);
        }
    }
}

int g_iteration = 0;
//return true if the SI/L value for this parameter changes from DontKnow to False
bool SIL::recomputeSILValue(SILParameter* silParameter, IELSection* ielSection)
{
    const std::vector<unsigned int>& rd = silParameter->getRD();
    for (unsigned int i = 0; i < rd.size(); ++i)
    {
        Instruction* inst;
        if ((inst = dyn_cast<Instruction>(silParameter->getDefinition(i))) == NULL)
        {
            continue;
        }
        
        for (User::op_iterator j = inst->op_begin(); j != inst->op_end(); ++j)
        {
            if (isa<Constant>(*j) || !isa<Instruction>(*j) || isa<BasicBlock>(*j)) continue;

            //this can never return NULL as all *j that are considered here are inside the loop 
            //and we have constructed an SILParameter object for all variables used inside the loop
            SILParameter* p = ielSection->getSILParameter(*j);

            if (p == NULL)
            { 
		std::cout << "error: " << getLineNumber(inst) << std::endl;
		inst->dump();
		std::cout << std::endl;
                j->get()->dump(); 
                silParameter->getInstruction()->dump();
                silParameter->getValue()->dump();
            }

            assert(p != NULL);

            if (p->getSILValue() == False)
            {
                silParameter->setSILValue(False, SILParameter::Step2a, inst, p);
                /*if (g_iteration == 0)
                {
                    silParameter->print();
                    std::cerr << std::endl;
                }
                */
                return true;
            }
        }
    }

    const std::vector<Instruction*>& cp = silParameter->getCP();
    for (std::vector<Instruction*>::const_iterator i = cp.begin(); i != cp.end(); ++i)
    {
        for (User::op_iterator j = (*i)->op_begin(); j != (*i)->op_end(); ++j)
        {
            //TODO: parameters to function calls are not instructions, so will be skipped here
            if (isa<Constant>(*j) || !isa<Instruction>(*j))
                continue;

            //this can never return NULL as all *j that are considered here are inside the loop
            //and we have constructed an SILParameter object for all variables used inside the loop
            SILParameter* p = ielSection->getSILParameter(*j);
 
            if (p == NULL)
            { 
		std::cout << "error: cp: \n";
		(*i)->dump();
		std::cout << std::endl;
                j->get()->dump(); 
                silParameter->getInstruction()->dump();
                silParameter->getValue()->dump();
            }


            assert(p != NULL);

            if (p->getSILValue() == False)
            {
                silParameter->setSILValue(False, SILParameter::Step2b, *i, p);
                /*
                if (g_iteration == 0)
                {
                    silParameter->print();
                    std::cerr << std::endl;
                }
                */
                return true;
            }
        }
    }
    
    return false;
}

void SIL::runStep2(IELSection* ielSection)
{
    ControlDependence& cd = getAnalysis<ControlDependence>();

    SILParameterList& silParameters = ielSection->getSILParameters();
    int changed = 0;
    int iteration = 0;

    do
    {
        changed = 0;

        for (SILParameterList::iterator i = silParameters.begin(); i != silParameters.end(); ++i)
        {
            SILParameter* currentParameter = *i;

            if (currentParameter->getSILValue() == DontKnow)
            {
                //TODO: can be made faster by storing the control dependences on an instruction basis than storing it in SILParameter
                //otherwise we will end up computing CPs multiple times

                computeCP(ielSection, currentParameter, cd);
                if (recomputeSILValue(currentParameter, ielSection))
                {
                    ++changed;
                }
            }
        }

        ++iteration;
        g_iteration = iteration;

    } while (changed != 0);
}

void SIL::runStep3(IELSection* ielSection)
{
    SILParameterList& silParameters = ielSection->getSILParameters();

    for (SILParameterList::iterator i = silParameters.begin(); i != silParameters.end(); ++i)
    {
        if ((*i)->getSILValue() == DontKnow)
        {
            (*i)->setSILValue(True);
        }
    }
}

//TODO: find suitable name
IELSection* SIL::createIELSection(Loop* loop)
{
    assert (loop->getHeader() == *loop->block_begin() && "First node is not the header!");

    IELSection* currentIELSection = new IELSection(loop, getId());
    std::map<Value*, int> opcount;

    bool foundInstructionsInBody = false;
    //skip the header of the loop
    for (LoopBase<BasicBlock, Loop>::block_iterator block = loop->block_begin() + 1; block != loop->block_end(); ++block)
    {
        currentIELSection->addBlock(*block);
        for (BasicBlock::iterator instr = (*block)->begin(); instr != (*block)->end(); ++instr)
        {
            if (isa<PHINode>(instr)) //don't analyze the phi instruction by itself unless it is used in another instruction
            {
                continue;
            }

            for (User::op_iterator def = instr->op_begin(); def != instr->op_end(); ++def)
            {
                Value* v = def->get();

                if (isa<BasicBlock>(v) || !isa<Instruction>(v))
                {
                    continue;
                }
                else if (isa<Constant>(v))
                {
                    continue;
                }

                foundInstructionsInBody = true;
                SILParameter* silParameter = new SILParameter(loop, v, instr);
                currentIELSection->addSILParameter(silParameter);
            }
        }
    }

    if (!foundInstructionsInBody && skipEmptyBodyLoops)
    {
        delete currentIELSection;
        return NULL;
    }

    return currentIELSection;
}

bool SIL::finalCheck(IELSection* ielSection)
{
    bool isIELSection = true;
    std::vector<BasicBlock*> blocks = ielSection->getBlocks();

    for (std::vector<BasicBlock*>::iterator block = blocks.begin(); block != blocks.end() && isIELSection; ++block)
    {
        for (BasicBlock::iterator instr = (*block)->begin(); instr != (*block)->end() && isIELSection; ++instr)
        {
            if (LoadInst *loadInst = dyn_cast<LoadInst>(instr))
            {
                Value* coreOperand;
                std::set<Value*> indices = findCoreOperand(loadInst->getPointerOperand(), &coreOperand);
                for (std::set<Value*>::iterator index = indices.begin(); index != indices.end(); ++index)
                {
                    if (isa<Constant>(*index) || !isa<Instruction>(*index)) continue;
                    SILParameter* parameter = ielSection->getSILParameter(*index);

                    if (Instruction *indexInstruction = dyn_cast<Instruction>(*index))
                    {
                        std::vector<BasicBlock*>::iterator where = std::find(blocks.begin(), blocks.end(), indexInstruction->getParent());
                        if (where == blocks.end())
                        {
                            //TODO:this means that the definition for this index is outside of the loop and its use is also outside the loop
                            //assert(parameter == NULL);
                            continue;
                        }
                    }

                    if (parameter == NULL)
                    { 
                        std::cout << "error: " << getLineNumber(instr) << std::endl;
                        (instr)->dump();
                        std::cout << instr->getParent()->getName().str() << std::endl;
                        std::cout << std::endl;
                        (*index)->dump(); 
                    }

                    assert(parameter != NULL);

                    if (parameter->getSILValue() == False)
                    {
                        isIELSection = false;
                        if (printRejected)
                        {
                            std::cerr << "Array index" << std::endl;
                            parameter->print();
                            std::cerr << std::endl;
                        }
                        //parameter->printSILValue();
                    }
                }
            }
            else if (BranchInst* branchInst = dyn_cast<BranchInst>(instr))
            {
                if (branchInst->isConditional())
                {
                    if (Instruction* condition = dyn_cast<Instruction>(branchInst->getCondition()))
                    {
#if 0    
                        for (User::op_iterator j = condition->op_begin(); j != condition->op_end(); ++j)
                        {
                            if (isa<Constant>(*j) || !isa<Instruction>(*j)) continue;

                            SILParameter* parameter = ielSection->getSILParameter(*j);

                            if (parameter == NULL)
                            {
                                std::cout << "error: " << getLineNumber(instr) << std::endl;
                                (instr)->dump();
                                std::cout << instr->getParent()->getName().str() << std::endl;
                                std::cout << std::endl;
                                (*j)->dump();
                                std::cout << getLineNumber(condition) << std::endl;
                                condition->dump();
                            }

                            assert(parameter != NULL);
                            if (parameter->getSILValue() == False)
                            {
                                if (printRejected)
                                {
                                    std::cerr << "Branch\n";
                                    parameter->print();
                                    std::cerr << std::endl;
                                }
                                isIELSection = false;
                            }
                        }
#else                        
                        if (isa<Constant>(condition)) continue;
                        SILParameter* parameter = ielSection->getSILParameter(condition);
                        
                        if (parameter == NULL)
                        {
                            std::cout << "error: " << getLineNumber(instr) << std::endl;
                            (instr)->dump();
                            std::cout << instr->getParent()->getName().str() << std::endl;
                            std::cout << getLineNumber(condition) << std::endl;
                            condition->dump();
                        }

                        assert(parameter != NULL);
                        if (parameter->getSILValue() == False)
                        {
                            if (printRejected)
                            {
                                std::cerr << "Branch\n";
                                parameter->print();
                                std::cerr << std::endl;
                            }
                            isIELSection = false;
                        }
#endif                        
                    }
                    else
                    {
			std::cout << "condition ";
			branchInst->getCondition()->dump();
			std::cout << " is not an instruction\n";
                        assert(false);
                    }
                }
            }
        }
    }

    if (isIELSection)
    {
        ielSection->setIELSection(isIELSection);
    }
    
    return isIELSection;
}

void getMaxDepth(Loop* loop, int &max);
int getMaxDepth(Loop* loop)
{
    int max = -1;
    getMaxDepth(loop, max);
    return max;
}

void getMaxDepth(Loop* loop, int &max)
{
    if (max < (int)loop->getLoopDepth())
    {
        max = loop->getLoopDepth();
    }
    
    const std::vector<Loop*>& subLoops = loop->getSubLoops();
    for (std::vector<Loop*>::const_iterator i = subLoops.begin(); i != subLoops.end(); ++i)
    {
        getMaxDepth(*i, max);
    }
}

void SIL::checkOuterLoops(Loop* loop)
{
    ++m_counts.totalLoops;
    assert(m_currentReachingDef->getCurrentFunction() == loop->getHeader()->getParent());

    IELSection* ielSection = checkLoop(loop);
    if (ielSection == NULL)
    {
        const std::vector<Loop*>& subLoops = loop->getSubLoops();
        for (std::vector<Loop*>::const_iterator i = subLoops.begin(); i != subLoops.end(); ++i)
        {
            checkOuterLoops(*i);
        }
    }
    else
    {
        int depth = getMaxDepth(loop);
        if ((int)m_histogram.size() < depth + 1)
        {
            m_histogram.resize(depth + 1);
            ++m_histogram[depth];
        }
    }
}

IELSection* SIL::checkLoop(Loop* loop)
{
    IELSection* ielSection = createIELSection(loop);
    if (ielSection == NULL) return NULL;

    runStep1(ielSection);
    runStep2(ielSection);
    runStep3(ielSection);

    finalCheck(ielSection);

    if (ielSection->isIELSection())
    {
        ++m_counts.afterFinalCheck;
        m_ielSections.push_back(ielSection);
        ielSection->printIELSection();
        return ielSection;
    }

    delete ielSection;
    return NULL;
}

bool SIL::runOnFunction(Function& function)
{
    m_currentReachingDef = &getAnalysis<ReachingDef>();
    LoopInfo& loopInfo = getAnalysis<LoopInfo>();

    std::string functionName = function.getName().str();

    m_counts = Counts();
    m_histogram.clear();

    assert(m_counts.totalLoops == 0);

    if (outerLoops)
    {
        for (LoopInfo::iterator i = loopInfo.begin(); i != loopInfo.end(); ++i)
        {
            Loop* loop = *i;
            checkOuterLoops(loop);
        }
    }
    else
    {
        std::vector<BasicBlock*> headers;
        std::map<Loop*, bool> visited;

        for (Function::iterator i = function.begin(); i != function.end(); ++i)
        {
            Loop* loop = loopInfo.getLoopFor(i);
            if (loop == NULL) continue;
            if (visited.find(loop) != visited.end()) continue;
            visited.insert(std::pair<Loop*, bool>(loop, true));

            for (std::vector<BasicBlock*>::iterator j = headers.begin(); j != headers.end(); ++j)
            {
                assert(*j != loop->getHeader());
            }

            headers.push_back(loop->getHeader());
            assert(m_currentReachingDef->getCurrentFunction() == loop->getHeader()->getParent());
            checkLoop(loop);
        }

        m_counts.totalLoops = visited.size();
    }

    if (printCount)
    {
        if (m_counts.totalLoops != 0)
        {
            std::cerr << "Loop count: " << m_counts.totalLoops << std::endl;
            std::cerr << "After final check: " << m_counts.afterFinalCheck << std::endl;
            for (unsigned int i = 1; i < m_histogram.size(); ++i)
            {
                std::cerr << "Loop depth: " << i << " " << m_histogram[i] << std::endl;
            }
        }
    }
    
    return false;
}

/*
void SIL::printNode(Loop* loop, char* id, bool isFilled)
{
    int lineNumber = getLineNumber(loop);
    assert(lineNumber != -1);

    const std::string& name = loop->getHeader()->getName().str() + id;
    const std::string& label = loop->getHeader()->getName().str();

    m_file << name;
    
    if (isFilled)
    {
        m_file << " [style=filled, label=\"" << label + ": ";
    }
    else
    {
        m_file << " [label=\"" << label + ": ";
    }

    m_file << lineNumber;
    m_file << "\"]\n";

}
*/
void SIL::printEdge(Loop* srcLoop, Loop* dstLoop, char* id)
{
    const std::string& src = srcLoop->getHeader()->getName().str() + id;
    const std::string& dst = dstLoop->getHeader()->getName().str() + id;
    m_file << src + "->" + dst + ";\n";
}

#if 0
void SIL::printLoopStructure(Loop* loop, char* id)
{
    std::string subgraph = "subgraph ";
    std::vector<Loop*> subLoops = loop->getSubLoops();
    for (std::vector<Loop*>::iterator i subLoops.begin(); i != subLoops.end(); ++i)
    {
        printLoopStructure(*i, id);

        m_file << subgraph + loop->getHeader()->getName().str() + id + "{\n";
    }
}
#endif

void SIL::printAdjacentLoops(Loop* srcLoop, LoopInfo& loopInfo, char* id)
{
    std::map<BasicBlock*, bool> visited;
    std::queue<BasicBlock*> blockQueue;

    blockQueue.push(srcLoop->getHeader());
    printNode(srcLoop, id, false);

    while (!blockQueue.empty())
    {
        BasicBlock* block = blockQueue.front();
        blockQueue.pop();

        visited[block] = true;
        for (succ_iterator succ = succ_begin(block); succ != succ_end(block); ++succ)
        {
            if (visited.find(*succ) == visited.end())
            {
                Loop* dstLoop = loopInfo.getLoopFor(*succ);

                if (dstLoop != NULL && dstLoop != srcLoop && !isAncestor(dstLoop, srcLoop))
                {
                    if (m_loopGraph.find(srcLoop) == m_loopGraph.end()
                            || m_loopGraph[srcLoop].find(dstLoop) == m_loopGraph[srcLoop].end())
                    {
                        printNode(dstLoop, id, false);
                        printEdge(srcLoop, dstLoop, id);
                        m_loopGraph[srcLoop].insert(dstLoop);
                    }
                }

                if (dstLoop == NULL || dstLoop == srcLoop)
                {
                    blockQueue.push(*succ);
                }
            }
        }
    }
}

bool SIL::isAncestor(Loop* dstLoop, Loop* srcLoop)
{
    Loop* parent = srcLoop;

    do
    {
        parent = parent->getParentLoop();
        
        if (parent == dstLoop) return true;

    } while (parent != NULL);

    return false;
}

void SIL::dump(void)
{
    std::cout << "Found " << m_ielSections.size() << " IE/L-section" << (m_ielSections.size() > 1 ? "s\n": "\n");
    for (std::vector<IELSection*>::iterator i = m_ielSections.begin(); i != m_ielSections.end(); ++i)
    {
        Loop* loop = (*i)->getLoop();
        std::cerr << "Loop header: " << loop->getHeader()->getName().str() << "\n\n";
        
        SILParameterList& parList = (*i)->getSILParameters();

        for (SILParameterList::iterator j = parList.begin(); j != parList.end(); ++j)
        {
            SILParameter* par = *j;
            std::cout << "\t" << "Value:       ";
	    (par->getValue())->dump();
	    //std::cout << "\n\tInstruction: " << *(par->getInstruction()) << std::endl;
            std::cout << "\t------------\n";
        }
    }
}

void SIL::getAnalysisUsage(AnalysisUsage& AU) const
{
    AU.setPreservesAll();
    AU.addRequired<ReachingDef>();
    AU.addRequired<ControlDependence>();
    AU.addRequired<LoopInfo>();
}

