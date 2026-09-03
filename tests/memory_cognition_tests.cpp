#include "genesis/cognition/workspace.hpp"
#include "genesis/cognition/persistence.hpp"
#include "genesis/memory/graph.hpp"
#include "genesis/memory/persistence.hpp"
#include "genesis/runtime/runtime.hpp"
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
 MemoryNode pattern{"party-pattern","genesis",d('9'),d('8'),"mixed",MemoryKind::pattern,Origin::inferred,IdentityScope::organism_private,ActivationState::available,.6,.5,0,3,3,0,{"pattern-only"}};require(graph.add(pattern),"add related pattern");require(graph.connect({"party-negative","party-pattern",.5,"same-event-class"}),"connect transitive memory");auto related=graph.related("party-positive","same-event-class",2,4);require(related.size()==2&&related[1].depth==2&&related[1].path_weight==.1,"bounded relationship traversal");
 auto activated=graph.activate({"party","music"},"birthday",4);require(activated.size()==2&&activated[0].id=="party-positive","associative activation");
 require(graph.activate({"never-seen"},"unmatched",4).empty(),"unmatched features activated unrelated memories");
 auto predictions=graph.predict({"party","music"},"birthday",4);require(!predictions.empty(),"experience prediction");
 require(graph.decay(.5,.1)&&graph.find("party-negative")->state==ActivationState::dormant,"declarative decay");
 require(graph.reinforce("party-negative",.5,10)&&graph.find("party-negative")->state==ActivationState::available,"relearning trace recovery");
 require(graph.consolidate("party-positive")&&graph.verify(),"consolidation and graph verification");
 const auto bytes=MemoryStore::serialize(graph); MemoryStoreError store_error;
 auto decoded=MemoryStore::deserialize(bytes,&store_error);require(decoded&&decoded->verify()&&decoded->size()==3&&decoded->related("party-positive","same-event-class",2,4).size()==2,"memory serialization round trip");
 auto corrupt=bytes;corrupt[corrupt.size()/2]^=1;require(!MemoryStore::deserialize(corrupt,&store_error)&&store_error.code==MemoryStoreErrorCode::corrupt_record,"corrupt memory accepted");
 const auto root=std::filesystem::temp_directory_path()/"genesis-memory-store-v1";std::error_code cleanup;std::filesystem::remove_all(root,cleanup);MemoryStore store(root);
 require(store.write(graph,"1.0.0",&store_error),"memory store write");auto restored=store.read("genesis","1.0.0",&store_error);require(restored&&restored->verify(),"memory store restore");
 require(store.write(graph,"1.0.0",&store_error),"idempotent memory write");MemoryGraph conflicting("genesis",8,12);require(conflicting.add(positive),"conflict graph setup");require(!store.write(conflicting,"1.0.0",&store_error)&&store_error.code==MemoryStoreErrorCode::conflicting_version,"immutable memory version conflict accepted");
 require(!store.read("../escape","1.0.0",&store_error)&&store_error.code==MemoryStoreErrorCode::invalid_identifier,"unsafe memory path accepted");std::filesystem::remove_all(root,cleanup);
 using namespace genesis::cognition;
 BeliefModel models("genesis",8); require(models.update({"world-1","room occupied",d('e'),ModelKind::world,.8,.2,1,"room-occupied",true,BeliefState::active,{},"camera-1",.9}),"world belief");require(!models.update({"world-1","replacement",d('e'),ModelKind::world,.8,.2,2,"room-occupied",true,BeliefState::active,{},"camera-1",.9}),"duplicate belief event replaced history"); require(models.update({"self-1","energy constrained",d('f'),ModelKind::self,.7,.3,1,"self-energy",true,BeliefState::active,{},"metabolism",1}),"self belief");
 require(models.update({"world-2","room unoccupied",d('6'),ModelKind::world,.75,.25,2,"room-occupied",false,BeliefState::active,{},"camera-2",.7}),"contradicting belief");require(models.contradictions("room-occupied").size()==2,"contradiction set");require(models.update({"world-3","verified room state",d('7'),ModelKind::world,.95,.05,3,"room-occupied",true,BeliefState::active,"world-1","validated-sensor",1}),"belief supersession");require(models.find("world-1")->state==BeliefState::superseded,"superseded belief remained active");require(models.retract("world-2",d('5'),4)&&models.contradictions("room-occupied").empty(),"belief retraction");require(models.assessed_confidence("world-3",3,10)&&*models.assessed_confidence("world-3",3,10)==.95&&models.verify(),"belief source quality or freshness assessment failed");
 auto belief_bytes=BeliefStore::serialize(models);BeliefStoreError belief_error;auto belief_copy=BeliefStore::deserialize(belief_bytes,&belief_error);require(belief_copy&&belief_copy->verify()&&belief_copy->find("world-1")->state==BeliefState::superseded&&belief_copy->find("world-2")->state==BeliefState::retracted,"belief model roundtrip failed");auto belief_corrupt=belief_bytes;belief_corrupt[belief_corrupt.size()/2]^=1;require(!BeliefStore::deserialize(belief_corrupt,&belief_error)&&belief_error.code==BeliefStoreErrorCode::corrupt_record,"corrupt belief snapshot accepted");auto belief_future=belief_bytes;belief_future[std::string_view{"GENESIS-BELIEF"}.size()]=2;belief_future.replace(belief_future.size()-64,64,genesis::runtime::sha256(std::string_view{belief_future}.substr(0,belief_future.size()-64)));require(!BeliefStore::deserialize(belief_future,&belief_error)&&belief_error.code==BeliefStoreErrorCode::unsupported_schema,"unsupported belief schema accepted");const auto belief_root=std::filesystem::temp_directory_path()/"genesis-belief-store-v1";std::filesystem::remove_all(belief_root,cleanup);BeliefStore belief_store(belief_root);require(belief_store.write(models,"1.0.0",&belief_error)&&belief_store.write(models,"1.0.0",&belief_error),"belief store idempotent write failed");auto belief_conflict=models;require(belief_conflict.update({"other-1","agent available",d('4'),ModelKind::other_agent,.6,.4,5,"agent-available",true,BeliefState::active,{},"network-observation",.6})&&!belief_store.write(belief_conflict,"1.0.0",&belief_error)&&belief_error.code==BeliefStoreErrorCode::conflicting_version,"conflicting belief version accepted");require(!belief_store.read("../escape","1.0.0",&belief_error)&&belief_error.code==BeliefStoreErrorCode::invalid_identifier,"unsafe belief path accepted");auto belief_restored=belief_store.read("genesis","1.0.0",&belief_error);require(belief_restored&&belief_restored->verify(),"belief store recovery failed");std::filesystem::remove_all(belief_root,cleanup);
 ConsciousWorkspace workspace(2);require(workspace.submit({"candidate-1","memory",d('1'),.9,.8,false,2}),"workspace candidate");require(workspace.submit({"candidate-2","perception",d('2'),.8,.9,false,1}),"workspace perception");require(workspace.submit({"candidate-3","reflex",d('3'),1,1,true,1}),"inhibited candidate");
 auto report=workspace.introspect();require(report.admitted==2&&report.inhibited==0&&report.uncertainty>0,"bounded introspection");
 std::cout<<"memory and cognition tests passed\n";return EXIT_SUCCESS;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
