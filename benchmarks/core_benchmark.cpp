#include "genesis/genesis.hpp"
#include <chrono>
#include <iostream>
int main(){constexpr int n=100000;genesis::ProvenanceLedger l;auto s=std::chrono::steady_clock::now();for(int i=0;i<n;++i)l.append(i,"e"+std::to_string(i),"o","tick",std::to_string(i));double t=std::chrono::duration<double>(std::chrono::steady_clock::now()-s).count();std::cout<<"events="<<n<<" seconds="<<t<<" events_per_second="<<n/t<<" verified="<<l.verify()<<'\n';return l.verify()?0:1;}
