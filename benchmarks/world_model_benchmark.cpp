#include "genesis/cognition/world_model.hpp"
#include "genesis/cognition/world_persistence.hpp"
#include "genesis/runtime/runtime.hpp"
#include <chrono>
#include <iostream>
#include <string>

int main() {
 using namespace genesis;
 constexpr std::size_t count=10000;
 cognition::WorldDynamics world("benchmark",count,count,count);
 const auto start=std::chrono::steady_clock::now();
 for(std::size_t i=0;i<count;++i){const auto n=std::to_string(i),cause=runtime::sha256("cause-"+n),effect=runtime::sha256("effect-"+n);if(!world.observe_state({"observation-"+n,"entity-"+n,"input",cause,runtime::sha256("observation-evidence-"+n),i,.9})||!world.register_hypothesis({"hypothesis-"+n,"input",cause,"output",effect,runtime::sha256("hypothesis-evidence-"+n),i,i,0,0,.6,.6})||!world.issue_prediction({"prediction-"+n,"hypothesis-"+n,"entity-"+n,effect,{},i+1,i+10,0,.6,cognition::PredictionState::pending}))return 1;}
 const auto built=std::chrono::steady_clock::now();
 for(std::size_t i=0;i<count;++i){const auto n=std::to_string(i);if(!world.resolve_prediction("prediction-"+n,i%2?runtime::sha256("effect-"+n):runtime::sha256("different-"+n),runtime::sha256("outcome-"+n),i+5))return 2;}
 const auto resolved=std::chrono::steady_clock::now();
 std::size_t simulations=0;for(std::size_t i=0;i<1000;++i)simulations+=world.simulate("input",runtime::sha256("cause-"+std::to_string(i)),4).size();
 const auto simulated=std::chrono::steady_clock::now();
 cognition::WorldDynamics graph("benchmark",1,3000,1);
 for(std::size_t depth=0;depth<1000;++depth){const auto n=std::to_string(depth),next=std::to_string(depth+1);if(!graph.register_hypothesis({"primary-"+n,"node",runtime::sha256(n),"node",runtime::sha256(next),runtime::sha256("primary-evidence-"+n),depth,depth,0,0,.9,.9})||!graph.register_hypothesis({"branch-"+n,"node",runtime::sha256(n),"branch",runtime::sha256(n),runtime::sha256("branch-evidence-"+n),depth,depth,0,0,.5,.5}))return 3;}
 std::size_t multi_paths=0;for(std::size_t i=0;i<1000;++i){const auto result=graph.simulate_intervention({"node",runtime::sha256(std::to_string(i))},{8,2,16,32,.01});multi_paths+=result.paths.size();}
 const auto multi_simulated=std::chrono::steady_clock::now();
 const auto bytes=cognition::WorldDynamicsStore::serialize(world);const auto restored=cognition::WorldDynamicsStore::deserialize(bytes);const auto ended=std::chrono::steady_clock::now();
 if(!world.verify()||simulations!=1000||!multi_paths||!restored||restored->prediction_count()!=count)return 4;
 std::cout<<"states="<<world.state_count()<<" hypotheses="<<world.hypothesis_count()<<" predictions="<<world.prediction_count()<<" build_seconds="<<std::chrono::duration<double>(built-start).count()<<" resolve_seconds="<<std::chrono::duration<double>(resolved-built).count()<<" simulations=1000 simulation_seconds="<<std::chrono::duration<double>(simulated-resolved).count()<<" multi_step_searches=1000 multi_step_paths="<<multi_paths<<" multi_step_seconds="<<std::chrono::duration<double>(multi_simulated-simulated).count()<<" snapshot_bytes="<<bytes.size()<<" persistence_roundtrip_seconds="<<std::chrono::duration<double>(ended-multi_simulated).count()<<'\n';
}
