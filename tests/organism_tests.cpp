#include "genesis/organism/anatomy.hpp"
#include "genesis/organism/systems.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

using namespace genesis::organism;
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::string digest(char c) { return std::string(64, c); }
ComputationalCell make_cell(std::string id, std::uint64_t compute = 10) {
    CellSpec spec{std::move(id), digest('a'), "generic", {"input"}, {"output"}, {}, {compute, 100, 2, 5}, .2, .9, 1.0, 0, digest('b')};
    ComputationalCell cell(std::move(spec));
    require(cell.transition(CellState::immature, "growth"), "created to immature");
    require(cell.transition(CellState::differentiating, "expression"), "immature to differentiating");
    require(cell.differentiate(CellType::neural, "neural-v1"), "differentiate neural");
    require(cell.transition(CellState::specialized, "mature"), "active to specialized");
    return cell;
}
}
int main() try {
    auto cell = make_cell("cell-1");
    require(cell.verify(), "cell audit verifies");
    require(cell.update_health(.6), "health update");
    require(cell.transition(CellState::damaged, "fault"), "damage");
    require(cell.transition(CellState::repairing, "isolated"), "repair start");
    require(cell.transition(CellState::specialized, "repair passed"), "repair complete");
    require(cell.transition(CellState::senescent, "retired"), "senescence");
    require(cell.recycle("controlled recycling"), "recycling");
    require(!cell.transition(CellState::active, "invalid"), "recycled is terminal");
    require(cell.verify(), "final cell audit verifies");

    auto cell2 = make_cell("cell-2");
    Tissue tissue({"neural-tissue", "1.0", TissueType::neural, {CellType::neural}, 2, {30, 1000, 20, 20}});
    require(tissue.add_cell(cell2), "cell joins tissue");
    require(tissue.verify() && tissue.health() == 1.0, "tissue verifies");
    Organ organ({"nervous-organ", "1.0", {"sensory"}, {"motor"}, {}, {"routing"}, {TissueType::neural}, 2, {100, 5000, 100, 100}});
    require(organ.add_tissue(tissue), "tissue joins organ");
    require(organ.verify(), "organ verifies");

    RepairWorkflow repair("repair-1", "cell-2", "health threshold");
    require(repair.advance(RepairStage::isolated, "routing blocked"), "isolate");
    require(repair.advance(RepairStage::diagnosed, "state inspected"), "diagnose");
    require(repair.advance(RepairStage::repair_applied, "state reconstructed"), "repair");
    require(repair.advance(RepairStage::verified, "regression passed"), "verify");
    require(repair.advance(RepairStage::reintegrated, "traffic restored"), "reintegrate");
    require(repair.terminal() && repair.verify(), "repair audit verifies");

    SignalRouter router(3);
    require(router.register_endpoint("cell-a") && router.register_endpoint("cell-b"), "endpoints");
    require(router.enqueue({"s1","cell-a","cell-b","work",digest('c'),SignalKind::excitatory,1,1,10}), "signal one");
    require(router.enqueue({"s2","cell-a","cell-b","urgent",digest('d'),SignalKind::priority,9,2,10}), "signal two");
    auto signals = router.drain("cell-b", 3, 10);
    require(signals.size() == 2 && signals.front().id == "s2", "priority order");
    require(router.enqueue({"s3","cell-a","cell-b","expires",digest('e'),SignalKind::stress,1,3,4}), "expiring signal");
    require(router.drain("cell-b", 4, 10).empty() && router.expired_count() == 1, "expiry");

    Metabolism metabolism({{MetabolicResource::cpu,100},{MetabolicResource::ram,1000},{MetabolicResource::energy,50}});
    { auto reservation = metabolism.reserve({{MetabolicResource::cpu,40},{MetabolicResource::energy,20}}); require(reservation.has_value(), "metabolic reservation"); require(metabolism.available(MetabolicResource::cpu)==60, "resources consumed"); }
    require(metabolism.available(MetabolicResource::cpu)==100, "RAII resource recovery");
    require(!metabolism.reserve({{MetabolicResource::cpu,101}}), "over-allocation rejected");

    HomeostasisController home;
    require(home.configure(Metric::system_load,{0,.1,.7,1}), "homeostasis band");
    require(home.evaluate(Metric::system_load,.5).level==PressureLevel::nominal, "nominal homeostasis");
    require(home.evaluate(Metric::system_load,1.1).action==CompensatoryAction::throttle_compute, "critical action");
    ModulationState modulation{}; ModulationState delta{}; delta.urgency=1.5; require(modulation.apply(delta) && modulation.urgency==1.0, "bounded modulation");

    ImmuneClassifier classifier;
    IntegrityObservation own{"organism","organism","owner","owner","provider",false,false,false,true,false,true,AuthenticationState::authenticated};
    require(classifier.classify(own)==IdentityClass::self, "self classification");
    own.digest_match=false; require(classifier.classify(own)==IdentityClass::corrupted_self, "corrupted self");
    IntegrityObservation external{"device","organism","owner","other","diagnostic",false,false,true,true,false,false,AuthenticationState::diagnostic_only};
    require(classifier.classify(external)==IdentityClass::untrusted_external, "unqualified provider cannot establish trust");
    ImmuneMemory memory(2);
    require(memory.remember({"sig-1","cause","isolate","test",IdentityClass::malicious_external,1}), "remember incident");
    require(!memory.remember({"sig-1","cause","isolate","test",IdentityClass::malicious_external,2}), "deduplicate incident");
    require(memory.quarantine("device") && memory.quarantined("device") && memory.release("device"), "quarantine lifecycle");
    std::cout << "organism tests passed\n";
    return EXIT_SUCCESS;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
