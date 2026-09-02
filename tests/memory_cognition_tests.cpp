#include "genesis/cognition/workspace.hpp"
#include "genesis/memory/graph.hpp"
#include "genesis/memory/persistence.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace { void require(bool v,const char*m){if(!v)throw std::runtime_error(m);} std::string d(char c){return std::string(64,c);} }
int main()try{
 using namespace genesis::memory;
 MemoryGraph graph("genesis",8,12);
 MemoryNode positive{"party-positive","genesis",d('a'),d('b'),"positive",MemoryKind::episode,Origin::direct_experience,IdentityScope::organism_private,ActivationState::available,.8,.9,.7,1,1,1,{"party","music","friends"}};
 MemoryNode negative{"party-negative","genesis",d('c'),d('d'),"negative",MemoryKind::episode,Origin::direct_experience,IdentityScope::organism_private,ActivationState::available,.7,.6,-.5,2,2,1,{"party","crowd","unknown"}};
 require(graph.add(positive)&&graph.add(negative),"add scoped memories");
 require(graph.connect({"party-positive","party-negative",.2,"same-event-class"}),"connect memories");
 auto activated=graph.activate({"party","music"},"birthday",4);require(activated.size()==2&&activated[0].id=="party-positive","associative activation");
 require(graph.activate({"never-seen"},"unmatched",4).empty(),"unmatched features activated unrelated memories");
 auto predictions=graph.predict({"party","music"},"birthday",4);require(!predictions.empty(),"experience prediction");
 require(graph.decay(.5,.1)&&graph.find("party-negative")->state==ActivationState::dormant,"declarative decay");
 require(graph.reinforce("party-negative",.5,10)&&graph.find("party-negative")->state==ActivationState::available,"relearning trace recovery");
 require(graph.consolidate("party-positive")&&graph.verify(),"consolidation and graph verification");
 const auto bytes=MemoryStore::serialize(graph); MemoryStoreError store_error;
 auto decoded=MemoryStore::deserialize(bytes,&store_error);require(decoded&&decoded->verify()&&decoded->size()==2,"memory serialization round trip");
 auto corrupt=bytes;corrupt[corrupt.size()/2]^=1;require(!MemoryStore::deserialize(corrupt,&store_error)&&store_error.code==MemoryStoreErrorCode::corrupt_record,"corrupt memory accepted");
 const auto root=std::filesystem::temp_directory_path()/"genesis-memory-store-v1";std::error_code cleanup;std::filesystem::remove_all(root,cleanup);MemoryStore store(root);
 require(store.write(graph,"1.0.0",&store_error),"memory store write");auto restored=store.read("genesis","1.0.0",&store_error);require(restored&&restored->verify(),"memory store restore");
 require(store.write(graph,"1.0.0",&store_error),"idempotent memory write");MemoryGraph conflicting("genesis",8,12);require(conflicting.add(positive),"conflict graph setup");require(!store.write(conflicting,"1.0.0",&store_error)&&store_error.code==MemoryStoreErrorCode::conflicting_version,"immutable memory version conflict accepted");
 require(!store.read("../escape","1.0.0",&store_error)&&store_error.code==MemoryStoreErrorCode::invalid_identifier,"unsafe memory path accepted");std::filesystem::remove_all(root,cleanup);
 using namespace genesis::cognition;
 BeliefModel models("genesis",4); require(models.update({"world-1","room occupied",d('e'),ModelKind::world,.8,.2,1}),"world belief"); require(models.update({"self-1","energy constrained",d('f'),ModelKind::self,.7,.3,1}),"self belief");
 ConsciousWorkspace workspace(2);require(workspace.submit({"candidate-1","memory",d('1'),.9,.8,false,2}),"workspace candidate");require(workspace.submit({"candidate-2","perception",d('2'),.8,.9,false,1}),"workspace perception");require(workspace.submit({"candidate-3","reflex",d('3'),1,1,true,1}),"inhibited candidate");
 auto report=workspace.introspect();require(report.admitted==2&&report.inhibited==0&&report.uncertainty>0,"bounded introspection");
 std::cout<<"memory and cognition tests passed\n";return EXIT_SUCCESS;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
