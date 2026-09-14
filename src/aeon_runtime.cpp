#include "aeon_runtime.h"
#include "aeon_engine.h"
#include <fstream>
#include <sstream>
#include <future>
#include <filesystem>

namespace Aeon {
using json = nlohmann::json;
static json command_json(const SimulationCommand& c) {
    return {{"id",c.id},{"cause",c.cause},{"actor",c.actor},{"due",c.due_year},
        {"priority",c.priority},{"source",int(c.source)},{"action",c.proposal.action_type},
        {"target",c.proposal.target_civ},{"confidence",c.proposal.confidence},
        {"utility",c.proposal.utility_score},{"duration",c.proposal.duration_years},
        {"kind",int(c.kind)},{"option",c.option},{"text",c.text},
        {"declaration",c.proposal.declaration},{"reasoning",c.proposal.reasoning},
        {"proposal_priority",c.proposal.priority}};
}
static SimulationCommand read_command(const json& j) {
    SimulationCommand c;
    c.actor=j.at("actor"); c.due_year=j.at("due"); c.priority=j.at("priority");
    c.source=CommandSource(j.at("source").get<int>()); c.cause=j.at("cause");
    c.proposal.action_type=j.at("action"); c.proposal.target_civ=j.at("target");
    c.proposal.confidence=j.at("confidence"); c.proposal.utility_score=j.at("utility");
    c.proposal.duration_years=j.at("duration");
    c.kind=CommandKind(j.value("kind",0));c.option=j.value("option",0);c.text=j.value("text",std::string{});
    c.proposal.declaration=j.value("declaration",std::string{});c.proposal.reasoning=j.value("reasoning",std::string{});
    c.proposal.priority=j.value("proposal_priority",0.5f);
    if(int(c.kind)<0 || int(c.kind)>int(CommandKind::CRISIS_RESPONSE) ||
        int(c.source)<0 || int(c.source)>int(CommandSource::ADMIN))throw std::invalid_argument("Invalid command kind or source");
    return c;
}
json world_metrics(const AeonEngine& e) {
    json nations=json::array();
    for(const auto& c:e.civs) nations.push_back({{"id",c.id},{"alive",c.is_alive},
        {"population",c.population.total},{"gdp",c.economy.gdp},{"income",c.economy.annual_income},
        {"army",c.army_size},{"stability",c.stability},{"unrest",c.unrest},
        {"government",int(c.government)},{"war",c.at_war},{"enemy",c.war_with_civ},
        {"research",c.tech.research_pts},{"territory",c.territory_tiles}});
    return {{"year",e.year},{"month",e.month},{"rng",e.rng.serialize()},
        {"nations",nations},{"history_count",e.history.event_count()},
        {"president",{{"active",e.president_game.active},{"nation",e.president_game.player_civ_id},
            {"treasury",e.president_game.treasury_gold},{"approval",e.president_game.approval_rating},
            {"crisis",e.president_game.crisis_pending},{"terms",e.president_game.term_counter}}}};
}
uint64_t SimulationRuntime::record(AeonEngine& e,std::string type,int actor,int target,
    std::string description,json effects) {
    const auto id=next_event++;
    events_.push_back({id,current_cause,e.year,actor,target,std::move(type),std::move(description),std::move(effects)});
    return id;
}
uint64_t SimulationRuntime::submit(AeonEngine& e,SimulationCommand c) {
    c.id=next_command++;
    if (!c.due_year) c.due_year=e.year;
    if (c.due_year < e.year) throw std::invalid_argument("Cannot schedule commands in the past");
    if (!replaying && phase == SimulationPhase::IDLE)
        journal_.push_back({{"kind","command"},{"year",e.year},{"command",command_json(c)}});
    pending.push_back(c);
    if (phase == SimulationPhase::IDLE && c.due_year == e.year) dispatch_due(e);
    return c.id;
}
void SimulationRuntime::dispatch_due(AeonEngine& e) {
    const auto previous=phase; phase=SimulationPhase::COMMANDS;
    std::stable_sort(pending.begin(),pending.end(),[](const auto&a,const auto&b){
        if(a.due_year!=b.due_year)return a.due_year<b.due_year;
        if(a.priority!=b.priority)return a.priority>b.priority;
        return a.id<b.id;
    });
    std::vector<SimulationCommand> due;
    auto end=std::find_if(pending.begin(),pending.end(),[&](const auto&c){return c.due_year>e.year;});
    due.assign(pending.begin(),end); pending.erase(pending.begin(),end);
    for(const auto& c:due) {
        const auto previous_cause=current_cause;
        current_cause=c.cause ? c.cause : previous_cause;
        current_cause=record(e,"COMMAND",c.actor,c.proposal.target_civ,c.proposal.action_type);
        if(c.kind == CommandKind::ACTION) e.apply_decision(c.actor,c.proposal);
        else {
            const auto before=world_metrics(e);
            if(c.source != CommandSource::PLAYER || c.actor != e.president_game.player_civ_id || !e.president_game.active)
                record(e,"REJECTED",c.actor,-1,"Presidential command requires the active player nation");
            else if(c.kind == CommandKind::PRESIDENTIAL_DECREE && c.option>=0 && c.option<=int(DecreeType::CUSTOM_DECREE)) {
                e.president_game.enact_decree(e,DecreeType(c.option),c.text);
                auto effects=json::diff(before,world_metrics(e));
                record(e,effects.empty()?"REJECTED":"APPLIED",c.actor,-1,"Presidential decree",effects);
            }else if(c.kind == CommandKind::CRISIS_RESPONSE && e.president_game.current_crisis.active &&
                c.option>=0 && c.option<(int)e.president_game.current_crisis.options.size()) {
                e.president_game.resolve_crisis_option(e,c.option);
                record(e,"APPLIED",c.actor,-1,"Crisis response",json::diff(before,world_metrics(e)));
            }else record(e,"REJECTED",c.actor,-1,"Invalid presidential command");
        }
        current_cause=previous_cause;
    }
    phase=previous;
}
void SimulationRuntime::begin_tick(AeonEngine& e) {
    if (phase != SimulationPhase::IDLE) throw std::logic_error("Reentrant simulation tick");
    if(!replaying) journal_.push_back({{"kind","tick"},{"year",e.year},{"month",e.month}});
    history_start_=e.history.event_count();
    current_cause=record(e,"TICK",-1,-1,"Begin annual simulation");
    phase=SimulationPhase::COMMANDS; dispatch_due(e); phase=SimulationPhase::WORLD;
}
void SimulationRuntime::think(AeonEngine& e) {
    phase=SimulationPhase::COGNITION;
    std::vector<std::pair<int,NationObservation>> observations;
    for(int i=0;i<(int)e.civs.size();++i) {
        if(e.civs[i].is_alive<=0 || e.civs[i].is_commons)continue;
        if(e.president_game.active && i==e.president_game.player_civ_id)continue;
        while(i>=(int)e.ai_controllers.size())e.ai_controllers.emplace_back(e.ai_controllers.size());
        observations.push_back({i,observe_nation(e.civs[i],e.civs,e.characters,e.history,e.ai_controllers[i],e.year)});
    }
    // All advisors see the same observation barrier; workers cannot mutate the world.
    std::vector<std::future<NationCognition>> jobs;
    if(parallel_advisors) for(const auto& entry:observations) {
        auto found=cognition.find(entry.first);
        NationCognition previous=found==cognition.end()?NationCognition{}:found->second;
        jobs.push_back(std::async(std::launch::async,[o=entry.second,previous]{return deliberate(o,&previous);}));
    }
    for(size_t k=0;k<observations.size();++k) {
        const auto& [i,o]=observations[k];
        auto& c=cognition[i];
        c=parallel_advisors?jobs[k].get():deliberate(o,&c);
        c.selected=AIDecision{};
        for(auto& p:c.proposals) {
            std::string reason;
            if(ActionValidator::validate(p.decision,e.civs[i],e.civs,e.year,
                e.ai_controllers[i].war_cooldown_,e.ai_controllers[i].trade_cooldown_,reason)) {
                c.selected=p.decision;break;
            }
            p.rejection_reason=reason;
        }
        e.ai_controllers[i].last_decision_log=explain(i);
        SimulationCommand cmd;cmd.actor=i;cmd.source=CommandSource::AI;cmd.proposal=c.selected;
        submit(e,cmd);
    }
    phase=SimulationPhase::CONSEQUENCES;dispatch_due(e);
}
void SimulationRuntime::end_tick(AeonEngine& e) {
    phase=SimulationPhase::HISTORY;
    for(size_t i=history_start_;i<e.history.all().size();++i) {
        const auto& h=e.history.all()[i];
        record(e,"HISTORY",h.civ_id,h.civ2_id,h.headline,
            {{"history_id",h.event_id},{"category",h.category},{"causes",h.causes},{"detail",h.detail}});
    }
    record(e,"WORLD_TICK",-1,-1,"Annual subsystem consequences",world_metrics(e));
    if(!replaying) journal_.push_back({{"kind","verify"},{"metrics",world_metrics(e)}});
    phase=SimulationPhase::IDLE;
    current_cause=0;
}
std::string SimulationRuntime::explain(int id) const {
    auto found=cognition.find(id);if(found==cognition.end())return "No cabinet session yet.";
    const auto& c=found->second;std::ostringstream s;
    s<<c.explanation<<"\nRuler: "<<c.observation.ruler<<" | "<<c.observation.ideology<<"\n";
    for(const auto&p:c.proposals)s<<p.advisor<<": "<<p.decision.action_type<<" ["<<p.decision.target_civ
        <<"] score="<<p.score<<" risk="<<p.risk<<"\n  Critic: "<<p.critique<<"\n";
    s<<"Selected: "<<c.selected.action_type;return s.str();
}
json SimulationRuntime::export_data() const {
    json result;result["events"]=json::array();result["pending"]=json::array();
    for(const auto&e:events_)result["events"].push_back({{"id",e.id},{"cause",e.cause},{"year",e.year},
        {"actor",e.actor},{"target",e.target},{"type",e.type},{"description",e.description},{"effects",e.effects}});
    for(const auto&c:pending)result["pending"].push_back(command_json(c));
    result["journal"]=journal_;return result;
}
bool SimulationRuntime::write_archive(const AeonEngine& e,const std::string& path,std::string& error)const {
    try {
        json archive={{"format","aeon-replay-1"},{"seed",e.seed},{"runtime",export_data()},
            {"metrics",world_metrics(e)},{"clock",{{"year",e.year},{"month",e.month},{"day",e.day},
                {"accumulator",e.time_accum},{"speed",e.speed},{"paused",e.paused}}}};
        std::ofstream out(path,std::ios::binary|std::ios::trunc);
        if(!out)throw std::runtime_error("Cannot open replay archive");
        out<<archive.dump(2);out.flush();if(!out)throw std::runtime_error("Archive write failed");
        return true;
    }catch(const std::exception&ex){error=ex.what();return false;}
}
bool SimulationRuntime::replay_archive(AeonEngine& e,const std::string& path,std::string& error) {
    try {
        std::ifstream in(path);json archive;in>>archive;
        if(archive.at("format")!="aeon-replay-1")throw std::runtime_error("Unsupported replay format");
        auto restored=std::make_unique<AeonEngine>();restored->init(archive.at("seed"));
        restored->runtime.replaying=true;
        for(const auto& entry:archive.at("runtime").at("journal")) {
            if(entry.at("kind")=="command") {
                restored->year=entry.at("year");restored->runtime.submit(*restored,read_command(entry.at("command")));
            }else if(entry.at("kind")=="tick") {
                restored->year=entry.at("year");restored->month=entry.at("month");restored->tick_one_year();
            }else if(entry.at("kind")=="verify" && world_metrics(*restored)!=entry.at("metrics"))
                throw std::runtime_error("Replay diverged at year "+std::to_string(restored->year));
        }
        if(archive.contains("clock")) {
            const auto& clock=archive.at("clock");
            restored->year=clock.at("year");restored->month=clock.at("month");restored->day=clock.at("day");
            restored->time_accum=clock.at("accumulator");restored->speed=clock.at("speed");restored->paused=clock.at("paused");
        }
        if(world_metrics(*restored)!=archive.at("metrics"))throw std::runtime_error("Archive final state mismatch");
        const auto regenerated=restored->runtime.export_data();
        if(regenerated.at("events")!=archive.at("runtime").at("events") ||
           regenerated.at("pending")!=archive.at("runtime").at("pending"))
            throw std::runtime_error("Archive event or scheduled-command mismatch");
        restored->runtime.journal_=archive.at("runtime").at("journal");
        restored->runtime.replaying=false;e=*restored;e.history.set_id_registry(&e.id_reg);
        return true;
    }catch(const std::exception&ex){error=ex.what();return false;}
}
void SimulationTimeline::checkpoint(const AeonEngine& e) {
    if(e.runtime.phase!=SimulationPhase::IDLE)throw std::logic_error("Checkpoint requires a tick boundary");
    if(!e.runtime.enabled)throw std::logic_error("Checkpoint requires deterministic cabinet mode");
    auto copy=std::make_shared<AeonEngine>(e);copy->history.set_id_registry(&copy->id_reg);
    snapshots_.push_back(std::move(copy));
}
bool SimulationTimeline::restore(AeonEngine& e,size_t index)const {
    if(index>=snapshots_.size())return false;
    e=*snapshots_[index];e.history.set_id_registry(&e.id_reg);return true;
}
std::unique_ptr<AeonEngine> SimulationTimeline::branch(size_t index)const {
    if(index>=snapshots_.size())return {};
    auto e=std::make_unique<AeonEngine>(*snapshots_[index]);e->history.set_id_registry(&e->id_reg);return e;
}
}
