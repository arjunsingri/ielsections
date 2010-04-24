#include "LoopGraph.h"

using namespace llvm;

char LoopGraph::ID = 0;
static RegisterPass<LoopGraph> loopGraphPass("loopgraph", "construct a graph of loops highlighting the IE/L-sections");

LoopGraph::LoopGraph()
    :   FunctionPass(&ID)
{
    //std::string filename = "/nfs/18/osu5369/llvm-2.6/lib/Analysis/ielsections/graph/asdf";
    std::string filename = "/home/singri/llvm-2.7/llvm/lib/Analysis/ielsections/graph/asdf";
    m_file.open(filename.c_str(), std::fstream::out);
    m_file << "digraph{\n";
}

LoopGraph::~LoopGraph(void)
{
    m_file << "}\n";
    m_file.close();
}

void LoopGraph::generateDotFile(std::string functionName)
{
    const SIL& sil = getAnalysis<SIL>();
    m_file << "subgraph cluster_"  << functionName << "{\n";
    m_file << "label = " << functionName << std::endl;

    for (std::map<Loop*, std::set<Loop*> >::iterator i = m_loopGraph.begin(); i != m_loopGraph.end(); ++i)
    {
        Loop* loop = (*i).first;
	assert(loop != NULL);
        std::set<Loop*>& dests = (*i).second;
        std::string src = loop->getHeader()->getName().str();
        m_file << src << " [label=\"" + src + "\"]\n";

        for (std::set<Loop*>::iterator j = dests.begin(); j != dests.end(); ++j)
        {
            std::string dest = (*j)->getHeader()->getName().str();
            m_file << dest << " [label=\"" + dest + "\"]\n";
            m_file << src << "->" << dest << ";" << std::endl;
        }
    }

    const std::vector<IELSection*>& ielSections = sil.getIELSections();
    for (std::vector<IELSection*>::const_iterator i = ielSections.begin(); i != ielSections.end(); ++i)
    {
        std::string iel = (*i)->getLoop()->getHeader()->getName().str();
        m_file << iel + " [style=filled, label=\"" + iel + "\"]\n";
    }

    m_file << "}\n";
}

void LoopGraph::constructLoopGraph(Function& function)
{
    const SIL& sil = getAnalysis<SIL>();
    LoopInfo& loopInfo = getAnalysis<LoopInfo>();
    const std::vector<Loop*>& loops = sil.getLoops();

    for (std::vector<Loop*>::const_iterator i = loops.begin(); i != loops.end(); ++i)
    {
        Loop* loop = *i;
        std::map<BasicBlock*, bool> visited;
        std::queue<BasicBlock*> blockQueue;

        assert(loop != NULL);
    
	for (LoopBase<BasicBlock, Loop>::block_iterator j = loop->block_begin(); j != loop->block_end(); ++j)
        {
            blockQueue.push(*j);
        }

        while (!blockQueue.empty())
        {
            BasicBlock* block = blockQueue.front();
            blockQueue.pop();

            for (succ_iterator succ = succ_begin(block); succ != succ_end(block); ++succ)
            {
                if (visited.find(*succ) == visited.end())
                {
                    Loop* containingLoop = loopInfo.getLoopFor(*succ);
                    if (containingLoop != NULL && containingLoop != loop)
                    {
                        m_loopGraph[loop].insert(containingLoop);
                    }

                    blockQueue.push(*succ);
		    visited[block] = true;
                }
            }
        }
    }
}

bool LoopGraph::runOnFunction(Function& function)
{
    constructLoopGraph(function);
    std::cout << function.getName().str() << std::endl;
    generateDotFile(function.getName().str());
    return false;
}

void LoopGraph::getAnalysisUsage(AnalysisUsage& AU) const
{
    AU.setPreservesAll();
    AU.addRequired<LoopInfo>();
    AU.addRequired<SIL>();
}

