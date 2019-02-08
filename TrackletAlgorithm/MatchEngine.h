#ifndef MATCHENGINE_H
#define MATCHENGINE_H

#include "Constants.hh"
#include "VMProjectionMemory.hh"
#include "CandidateMatchMemory.hh"
#include "VMStubMEMemory.hh"


void MatchEngine(const BXType  bx,
		 const VMStubMEMemory<BARRELPS>* const instubdata, 
		 const VMProjectionMemory<BARREL>* const inprojdata,
		 CandidateMatchMemory* const outcanddata
		 );

#endif