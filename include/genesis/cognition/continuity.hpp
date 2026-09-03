#pragma once
#include "genesis/identity/lineage.hpp"
#include "genesis/memory/graph.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace genesis::cognition {
struct ContinuityAnchor { OriginKind origin{OriginKind::genesis};std::string source_organism_id,source_checkpoint_digest; };
struct LifeEvent { std::string id,memory_id,content_digest,provenance_digest,previous_event_digest,event_digest;memory::Origin memory_origin{memory::Origin::observed};memory::IdentityScope scope{memory::IdentityScope::organism_private};std::uint64_t sequence{},logical_time{}; };
struct ContinuityCheckpoint { std::string id,head_event_digest,checkpoint_digest;std::uint64_t sequence{},logical_time{}; };
struct SegmentAudit { std::size_t gaps{},conflicts{},invalid_origins{},invalid_digests{};[[nodiscard]] bool clean()const noexcept{return !gaps&&!conflicts&&!invalid_origins&&!invalid_digests;} };
class AutobiographicalContinuity final {
public:
 AutobiographicalContinuity(LineageIdentity identity,ContinuityAnchor anchor,std::size_t event_capacity,std::size_t checkpoint_capacity);
 bool append(std::string event_id,const memory::MemoryNode& memory,std::uint64_t sequence,std::uint64_t logical_time,std::string* error=nullptr);
 bool checkpoint(std::string id,std::uint64_t logical_time,std::string* error=nullptr);
 [[nodiscard]] SegmentAudit inspect_segment(const std::vector<LifeEvent>& segment)const;
 [[nodiscard]] bool verify()const;
 [[nodiscard]] const LineageIdentity& identity()const noexcept{return identity_;}
 [[nodiscard]] const ContinuityAnchor& anchor()const noexcept{return anchor_;}
 [[nodiscard]] const std::vector<LifeEvent>& events()const noexcept{return events_;}
 [[nodiscard]] const std::vector<ContinuityCheckpoint>& checkpoints()const noexcept{return checkpoints_;}
private:LineageIdentity identity_;ContinuityAnchor anchor_;std::size_t event_capacity_{},checkpoint_capacity_{};std::vector<LifeEvent> events_;std::vector<ContinuityCheckpoint> checkpoints_;
};
}
