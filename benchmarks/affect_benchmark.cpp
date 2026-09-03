#include "genesis/cognition/affect.hpp"
#include "genesis/runtime/runtime.hpp"
#include <chrono>
#include <iostream>
#include <string>
int main(){using namespace genesis;cognition::AffectRegulator affect("benchmark",10000,10000);const auto start=std::chrono::steady_clock::now();for(std::size_t i=0;i<10000;++i){const auto n=std::to_string(i);if(!affect.ingest({"signal-"+n,"benchmark",runtime::sha256("evidence-"+n),cognition::AffectSource::memory,i%2?.4:-.4,.6,.3,.8,i}))return 1;if(i%2==0&&!affect.regulate("regulation-"+n,cognition::RegulationKind::grounding,.2,runtime::sha256("regulation-evidence-"+n),i))return 2;}const auto end=std::chrono::steady_clock::now();if(!affect.verify())return 3;std::cout<<"signals="<<affect.signals().size()<<" regulations="<<affect.regulations().size()<<" seconds="<<std::chrono::duration<double>(end-start).count()<<'\n';}
