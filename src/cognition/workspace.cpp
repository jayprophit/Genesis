#include "genesis/cognition/workspace.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace genesis::cognition { namespace { bool ok(std::string_view s){return !s.empty()&&s.size()<=1024;} bool ratio(double v){return std::isfinite(v)&&v>=0&&v<=1;} }
BeliefModel::BeliefModel(std::string id,std::size_t c):owner_id_(std::move(id)),capacity_(c){if(!ok(owner_id_)||!c)throw std::invalid_argument("invalid belief model");}
bool BeliefModel::update(Belief b){if(!ok(b.id)||!ok(b.proposition)||b.evidence_digest.size()!=64||!ratio(b.confidence)||!ratio(b.uncertainty))return false;auto it=std::find_if(beliefs_.begin(),beliefs_.end(),[&](const auto&x){return x.id==b.id;});if(it!=beliefs_.end()){if(b.observed_at<it->observed_at)return false;*it=std::move(b);return true;}if(beliefs_.size()>=capacity_)return false;beliefs_.push_back(std::move(b));return true;}
const Belief* BeliefModel::find(std::string_view id)const noexcept{auto it=std::find_if(beliefs_.begin(),beliefs_.end(),[&](const auto&b){return b.id==id;});return it==beliefs_.end()?nullptr:&*it;}
std::vector<Belief> BeliefModel::contradictions(std::string_view p)const{std::vector<Belief> out;for(const auto&b:beliefs_)if(b.proposition==p&&b.confidence>.5)out.push_back(b);return out;}
ConsciousWorkspace::ConsciousWorkspace(std::size_t c):capacity_(c){if(!c)throw std::invalid_argument("workspace capacity must be positive");}
bool ConsciousWorkspace::submit(WorkspaceItem i){if(!ok(i.id)||!ok(i.source)||i.payload_digest.size()!=64||!ratio(i.salience)||!ratio(i.confidence)||std::any_of(items_.begin(),items_.end(),[&](const auto&x){return x.id==i.id;}))return false;items_.push_back(std::move(i));std::sort(items_.begin(),items_.end(),[](const auto&a,const auto&b){const double as=a.inhibited?-1:a.salience*a.confidence,bs=b.inhibited?-1:b.salience*b.confidence;return as!=bs?as>bs:a.timestamp<b.timestamp;});if(items_.size()>capacity_)items_.resize(capacity_);return true;}
std::vector<WorkspaceItem> ConsciousWorkspace::focus()const{std::vector<WorkspaceItem> out;for(const auto&i:items_)if(!i.inhibited)out.push_back(i);return out;}
IntrospectionReport ConsciousWorkspace::introspect()const{IntrospectionReport r;r.candidates=items_.size();for(const auto&i:items_){if(i.inhibited)++r.inhibited;else{++r.admitted;r.mean_confidence+=i.confidence;}}if(r.admitted)r.mean_confidence/=r.admitted;r.uncertainty=1-r.mean_confidence;return r;}
void ConsciousWorkspace::clear()noexcept{items_.clear();}
}
