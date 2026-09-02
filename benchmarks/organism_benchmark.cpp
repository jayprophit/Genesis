#include "genesis/organism/systems.hpp"
#include <chrono>
#include <iostream>
#include <string>
int main() {
    constexpr std::size_t count=100000;
    genesis::organism::SignalRouter router(count);
    router.register_endpoint("source"); router.register_endpoint("target");
    const auto start=std::chrono::steady_clock::now();
    for(std::size_t i=0;i<count;++i) router.enqueue({"signal-"+std::to_string(i),"source","target","benchmark",std::string(64,'a'),genesis::organism::SignalKind::excitatory,static_cast<std::uint8_t>(i%10),i,i+count+1});
    const auto drained=router.drain("target",count,count);
    const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout << "signals=" << drained.size() << " seconds=" << elapsed << " signals_per_second=" << drained.size()/elapsed << '\n';
    return drained.size()==count?0:1;
}
