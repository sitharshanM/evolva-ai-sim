#pragma once
#include "aeon_engine.h"
#include <sstream>
#include <filesystem>
#include <stdexcept>
#include <fstream>

inline bool run_runtime_tests() {
    using namespace Aeon;
    int checks = 0;
    auto check = [&](bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);};
    std::ostringstream quiet;
    auto* output=std::cout.rdbuf(quiet.rdbuf());
    try {
        AeonRandom a(55);a.normal();a.uniform_int(0,100);
        AeonRandom b; b.deserialize(a.serialize());
        check(a.raw()==b.raw(),"RNG round trip failed");
        auto e=std::make_unique<AeonEngine>();e->init(42);
        auto observation=observe_nation(e->civs[0],e->civs,e->characters,e->history,e->ai_controllers[0],e->year);
        const auto independent=deliberate(observation);
        e->civs[1].army_size*=100;
        check(deliberate(observation).selected.action_type==independent.selected.action_type,
            "Advisor reads hidden world state");
        e->init(42);
        SimulationTimeline timeline;timeline.checkpoint(*e);
        e->year++;e->tick_one_year();
        const auto expected=world_metrics(*e);
        auto branched=timeline.branch(0);
        branched->runtime.parallel_advisors=true;
        branched->year++;branched->tick_one_year();
        check(world_metrics(*branched)==expected,"Parallel and serial execution diverged");
        check(timeline.restore(*e,0),"Rollback failed");
        e->year++;e->tick_one_year();
        check(world_metrics(*e)==expected,"Checkpoint continuation diverged");
        std::string error;
        const auto path=(std::filesystem::temp_directory_path()/"aeon-runtime-test-replay.json").string();
        check(e->runtime.write_archive(*e,path,error),error.c_str());
        auto replayed=std::make_unique<AeonEngine>();
        check(SimulationRuntime::replay_archive(*replayed,path,error),error.c_str());
        check(world_metrics(*replayed)==expected,"Durable replay diverged");
        nlohmann::json damaged;
        {std::ifstream in(path);in>>damaged;}
        damaged["runtime"]["events"][0]["description"]="tampered";
        {std::ofstream out(path);out<<damaged;}
        check(!SimulationRuntime::replay_archive(*replayed,path,error),"Tampered event archive accepted");
        check(world_metrics(*replayed)==expected,"Failed replay mutated destination");
        std::filesystem::remove(path);
        SimulationCommand delayed;
        delayed.actor=0;delayed.due_year=e->year+1;delayed.proposal.action_type="RESEARCH";
        auto research=e->civs[0].tech.research_pts;
        e->runtime.submit(*e,delayed);
        check(e->civs[0].tech.research_pts==research,"Delayed command executed early");
        e->year++;e->runtime.dispatch_due(*e);
        check(e->civs[0].tech.research_pts==research+60,"Delayed command missing");
        check(!e->runtime.events().empty() && e->runtime.events().back().cause!=0,"Consequence has no cause");
        check(branched->year!=e->year,"Branch shared state");
        std::cout.rdbuf(output);std::cout<<checks<<" runtime checks passed\n";return true;
    }catch(const std::exception& ex){
        std::cout.rdbuf(output);std::cerr<<"Runtime test failed: "<<ex.what()<<'\n';return false;
    }
}
