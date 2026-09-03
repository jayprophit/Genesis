#include "genesis/cognition/capability.hpp"
#include "genesis/runtime/runtime.hpp"
#include <chrono>
#include <iostream>
#include <string>
int main(){using namespace genesis;constexpr std::size_t count=10000;cognition::SelfCapabilityModel model("benchmark",count,{5,.8,2});const auto start=std::chrono::steady_clock::now();for(std::size_t i=0;i<count;++i){const auto n=std::to_string(i);if(!model.declare_capability("cap-"+n,"local-route-"+n,runtime::sha256("declaration-"+n),i))return 1;for(std::size_t trial=0;trial<5;++trial)if(!model.record_outcome("cap-"+n,true,runtime::sha256("outcome-"+n+"-"+std::to_string(trial)),i+trial+1))return 2;if(!model.qualify("cap-"+n,runtime::sha256("qualification-"+n),i+6))return 3;}const auto end=std::chrono::steady_clock::now();if(!model.verify())return 4;std::cout<<"capabilities="<<count<<" trials="<<count*5<<" qualified="<<count<<" seconds="<<std::chrono::duration<double>(end-start).count()<<'\n';}
