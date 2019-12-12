//============================================================================
// Name        : superScalalr.cpp
// Author      : Pratik Suresh Raut
// Version     :
// Copyright   : ignore warning and term and conditions just like you ignore your life problems
// Description : Hello World in C++, Ansi-style
//============================================================================

#include "defination.h"
#include "pipeline.h"

int main(int argc, char *argv[])
{
    //Local variable declaration
    char *trace_file = NULL;            // to store file name from command line
    //Expected input pattern
    /*
     *sim <S> <N> <BLOCKSIZE> <L1_size> <L1_ASSOC> <L2_SIZE> <L2_ASSOC> <tracefile>
     */
    if (argc != 9) // Checks if correct number of inputs have been given.
    {
        // Throw error and exit if wrong
        cout << "Error: Given inputs:" << argc - 1 << endl;
        return 1;
    }

    //open the trace file
    trace_file = argv[8];

    pipeline *sim = new pipeline(atoi(argv[1]), atoi(argv[2]));
    if ((atoi(argv[3]) != 0) && (atoi(argv[4]) != 0) && (atoi(argv[5]) != 0))
    {
        sim->cacheL1 = new BranchBuffer(atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
        sim->isL1Present = true;
    }

    if ((atoi(argv[6]) != 0) && (atoi(argv[7]) != 0))
    {
        sim->cacheL2 = new BranchBuffer(atoi(argv[3]), atoi(argv[6]), atoi(argv[7]));
        sim->isL2Present = true;
    }

    sim->pFileHandler = fopen(trace_file, "r");
    //If FP == NULL then there is issue will opening trace_file
    if (sim->pFileHandler == NULL)
    {
        // Throw error and exit if fopen() failed
        cout << "Error: Unable to open file " << trace_file << endl;
        delete sim;
        return 1;
    }

    do
    {
        sim->retire();
        sim->executeCycle();
        sim->issueCycle();
        sim->decodeCycle();
        sim->fetchCycle();
    }
    while (!sim->Advance_cycle());

    //L1 print contents
    if(sim->isL1Present)
    {
        cout << "L1 CACHE CONTENTS" << endl;
        sim->cacheL1->printBufferContet();
        cout << endl;
    }

    //L2 print contents
    if(sim->isL2Present)
    {
        cout << "L2 CACHE CONTENTS" << endl;
        sim->cacheL2->printBufferContet();
        cout << endl;
    }

    //output print
    sim->printFunc();

    //delete cache pointer
    if(sim->isL1Present)
        sim->cacheL1->~BranchBuffer();
    if(sim->isL2Present)
        sim->cacheL2->~BranchBuffer();
    // destory pipeline pointer
    sim->~pipeline();

    return 0;
}
