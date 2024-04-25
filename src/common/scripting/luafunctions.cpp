/* Conquer Space
 * Copyright (C) 2021-2023 Conquer Space
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "common/scripting/luafunctions.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "common/components/area.h"
#include "common/components/bodies.h"
#include "common/components/coordinates.h"
#include "common/components/economy.h"
#include "common/components/event.h"
#include "common/components/infrastructure.h"
#include "common/components/name.h"
#include "common/components/orbit.h"
#include "common/components/organizations.h"
#include "common/components/player.h"
#include "common/components/population.h"
#include "common/components/resource.h"
#include "common/components/science.h"
#include "common/components/ships.h"
#include "common/components/surface.h"
#include "common/scripting/functionreg.h"
#include "common/systems/actions/cityactions.h"
#include "common/systems/actions/factoryconstructaction.h"
#include "common/systems/actions/shiplaunchaction.h"
#include "common/systems/economy/markethelpers.h"
#include "common/systems/science/labs.h"
#include "common/systems/science/technology.h"
#include "common/util/random/stdrandom.h"
#include "common/util/utilnumberdisplay.h"

namespace components = cqsp::common::components;
namespace bodies = components::bodies;
namespace ships = components::ships;
namespace types = components::types;
namespace actions = cqsp::common::systems::actions;
namespace infrstructure = components::infrastructure;
using bodies::Body;

namespace cqsp::common::scripting {
/// <summary>
/// Initializes functions for RNG
/// </summary>
/// <param name="app"></param>
void FunctionRandom(Universe& universe, ScriptInterface& script_engine) {
    // RNG
    // Make namespace
    CREATE_NAMESPACE(core);

    REGISTER_FUNCTION("random", [&](int low, int high) { return universe.random->GetRandomInt(low, high); });

    REGISTER_FUNCTION("random_normal_int",
                      [&](int mean, int sd) { return universe.random->GetRandomNormal(mean, sd); });
}

void FunctionUniverseBodyGen(Universe& universe, ScriptInterface& script_engine) {
    CREATE_NAMESPACE(core);

    REGISTER_FUNCTION("add_planet", [&]() {
        entt::entity planet = universe.create();
        auto& body = universe.emplace<Body>(planet);
        universe.emplace<bodies::Planet>(planet);
        return planet;
    });

    REGISTER_FUNCTION("add_star", [&]() {
        entt::entity star = universe.create();
        universe.emplace<bodies::Star>(star);
        auto& body = universe.emplace<Body>(star);
        universe.emplace<bodies::LightEmitter>(star);
        return star;
    });

    REGISTER_FUNCTION("set_orbit", [&](entt::entity orbital_entity, double semi_major_axis, double eccentricity,
                                       double inclination, double LAN, double w, double M0) {
        types::Orbit& orb = universe.emplace<types::Orbit>(orbital_entity);
        orb.eccentricity = eccentricity;
        orb.semi_major_axis = semi_major_axis;
        orb.inclination = inclination;
        orb.LAN = LAN;
        orb.w = w;
        orb.M0 = M0;
        auto& kinematics = universe.emplace<types::Kinematics>(orbital_entity);
        types::UpdatePos(kinematics, orb);
    });

    REGISTER_FUNCTION("set_system_position", [&](entt::entity orbital_ent, double x, double y) {
        auto& coord = universe.get_or_emplace<types::GalacticCoordinate>(orbital_ent);
        coord.x = x;
        coord.y = y;
    });

    REGISTER_FUNCTION("set_radius", [&](entt::entity body, int radius) {
        Body& bod = universe.get<Body>(body);
        bod.radius = radius;
    });

    REGISTER_FUNCTION("create_terrain", [&](entt::entity planet, int seed, entt::entity terrain_type) {
        static_cast<void>(universe.emplace<bodies::Terrain>(planet, seed, terrain_type));
    });
}

void FunctionCivilizationGen(Universe& universe, ScriptInterface& script_engine) {
    CREATE_NAMESPACE(core);

    REGISTER_FUNCTION("add_civilization", [&]() {
        entt::entity civ = universe.create();
        universe.emplace<components::Organization>(civ);
        return civ;
    });

    REGISTER_FUNCTION("set_owner", [&](entt::entity entity, entt::entity owner) {
        auto& gov = universe.get_or_emplace<components::Governed>(entity);
        gov.governor = owner;
    });

    REGISTER_FUNCTION("is_player",
                      [&](entt::entity civ) { return static_cast<bool>(universe.all_of<components::Player>(civ)); });

    REGISTER_FUNCTION("add_planet_habitation",
                      [&](entt::entity planet) { universe.emplace<components::Habitation>(planet); });

    REGISTER_FUNCTION("add_planet_settlement", [&](entt::entity planet, double lat, double longi) {
        return actions::CreateCity(universe, planet, lat, longi);
    });
}

void FunctionEconomy(Universe& universe, ScriptInterface& script_engine) {
    CREATE_NAMESPACE(core);

    REGISTER_FUNCTION("create_industries", [&](entt::entity city) { universe.emplace<components::IndustrialZone>(city); });

    REGISTER_FUNCTION("add_industry", [&](entt::entity city, entt::entity entity) {
        universe.get<components::IndustrialZone>(city).industries.push_back(entity);
    });

    REGISTER_FUNCTION("create_factory", [&](entt::entity city, entt::entity recipe, float productivity) {
        entt::entity factory = actions::CreateFactory(universe, city, recipe, productivity);
        return factory;
    });

    REGISTER_FUNCTION("add_production", [&](entt::entity factory) {
        // Factory will produce in the first tick
        universe.emplace<components::FactoryProducing>(factory);
    });

    REGISTER_FUNCTION("set_power_consumption", [&](entt::entity factory, double max, double min) {
        universe.emplace<components::infrastructure::PowerConsumption>(factory, max, min, 0.f);
        return factory;
    });

    REGISTER_FUNCTION("add_power_plant", [&](entt::entity city, double productivity) {
        entt::entity entity = universe.create();
        universe.emplace<components::infrastructure::PowerPlant>(entity, productivity);
        universe.get<components::IndustrialZone>(city).industries.push_back(entity);
        return entity;
    });

    REGISTER_FUNCTION("create_commercial_area",
                      [&](entt::entity city) { return actions::CreateCommercialArea(universe, city); });

    REGISTER_FUNCTION("set_resource_consume", [&](entt::entity entity, entt::entity good, double amount) {
        auto& consumption = universe.get_or_emplace<components::ResourceConsumption>(entity);
        consumption[good] = amount;
    });

    REGISTER_FUNCTION("set_resource", [&](entt::entity planet, entt::entity resource, int seed) {
        auto& dist = universe.get_or_emplace<components::ResourceDistribution>(planet);
        dist.dist[resource] = seed;
    });

    // TODO(EhWhoAmI): Will have to fix the documentation for this so that it looks neater
    auto lambda = [&]() {
        /*entt::entity entity = universe.create();
        auto& market = universe.emplace<cqspc::Market>(entity);
        universe.emplace<cqspc::ResourceStockpile>(entity);
        // Set the market prices
        // TODO(EhWhoAmI): This is a bandaid solution, please fix this

        auto view = universe.view<cqspc::Good, cqspc::Price>();
        for (entt::entity entity : view) {
            // Assign price to market
            market.prices[entity] = universe.get<cqspc::Price>(entity);
        }*/
        entt::entity market_entity = cqsp::common::systems::economy::CreateMarket(universe);
        // Set prices of market
        auto view = universe.view<components::Good, components::Price>();
        auto& market = universe.get<cqsp::common::components::Market>(market_entity);
        for (entt::entity entity : view) {
            // Assign price to market
            market.market_information[entity].price = universe.get<components::Price>(entity);
        }
        return market_entity;
        // return entity;
    };
    REGISTER_FUNCTION("create_market", lambda);

    REGISTER_FUNCTION("place_market", [&](entt::entity market, entt::entity planet) {
        universe.emplace<components::MarketCenter>(planet, market);
    });

    REGISTER_FUNCTION("attach_market", [&](entt::entity market_entity, entt::entity participant) {
        cqsp::common::systems::economy::AddParticipant(universe, market_entity, participant);
    });

    REGISTER_FUNCTION("add_cash", [&](entt::entity participant, double balance) {
        universe.get_or_emplace<components::Wallet>(participant) += balance;
    });
}

void FunctionUser(Universe& universe, ScriptInterface& script_engine) {
    CREATE_NAMESPACE(core);

    REGISTER_FUNCTION("set_name", [&](entt::entity entity, std::string name) {
        universe.emplace_or_replace<components::Name>(entity, name);
    });

    REGISTER_FUNCTION("to_human_string", [&](int64_t number) { return cqsp::common::util::LongToHumanString(number); });

    REGISTER_FUNCTION("get_name", [&](entt::entity entity) { return universe.get<components::Name>(entity).name; });

    REGISTER_FUNCTION("get_random_name", [&](const std::string& name_gen, const std::string& rule) {
        return universe.name_generators[name_gen].Generate(rule);
    });
}

void FunctionPopulation(cqsp::common::Universe& universe, ScriptInterface& script_engine) {
    CREATE_NAMESPACE(core);

    REGISTER_FUNCTION("add_population_segment", [&](entt::entity settlement, uint64_t popsize) {
        entt::entity population = universe.create();
        universe.emplace<components::PopulationSegment>(population, popsize);
        universe.emplace<components::ResourceStockpile>(population);
        universe.emplace<components::LaborInformation>(population);
        // Add to planet list
        universe.get<components::Settlement>(settlement).population.push_back(population);

        return population;
    });

    REGISTER_FUNCTION("get_segment_size", [&](entt::entity segment) {
        return universe.get<components::PopulationSegment>(segment).population;
    });

    // Get population segments of a planet
    REGISTER_FUNCTION("get_segments",
                      [&](entt::entity planet) { return universe.get<components::Settlement>(planet).population; });

    // Get cities of a planet
    REGISTER_FUNCTION("get_cities",
                      [&](entt::entity planet) { return universe.get<components::Habitation>(planet).settlements; });
}

void FunctionShips(Universe& universe, ScriptInterface& script_engine) {
    CREATE_NAMESPACE(core);

    REGISTER_FUNCTION("create_ship", [&](entt::entity civ, entt::entity orbit, entt::entity starsystem) {
        return actions::CreateShip(universe, civ, orbit, starsystem);
    });
}

void FunctionEvent(cqsp::common::Universe& universe, ScriptInterface& script_engine) {
    CREATE_NAMESPACE(core);

    REGISTER_FUNCTION("push_event", [&](entt::entity entity, sol::table event_table) {
        auto& queue = universe.get_or_emplace<event::EventQueue>(entity);
        auto event = std::make_shared<event::Event>();
        event->title = event_table["title"];
        SPDLOG_INFO("Parsing event \"{}\"", event->title);
        event->content = event_table["content"];
        event->image = event_table["image"];
        sol::optional<std::vector<sol::table>> optional = event_table["actions"];
        if (optional) {
            for (auto& action : *optional) {
                if (action == sol::nil) {
                    continue;
                }
                auto event_result = std::make_shared<cqsp::common::event::EventResult>();
                event_result->name = action["name"];
                sol::optional<std::string> tooltip = action["tooltip"];
                if (tooltip) {
                    event_result->tooltip = *tooltip;
                }

                event->table = event_table;
                sol::optional<sol::function> f = action["action"];
                event_result->has_event = f.has_value();
                if (f) {
                    event_result->action = *f;
                }
                event->actions.push_back(event_result);
            }
        }
        queue.events.push_back(event);
    });
}

void FunctionResource(cqsp::common::Universe& universe, ScriptInterface& script_engine) {
    CREATE_NAMESPACE(core);

    REGISTER_FUNCTION("add_resource", [&](entt::entity storage, entt::entity resource, int amount) {
        // Add resources to the resource stockpile
        universe.get<components::ResourceStockpile>(storage)[resource] += amount;
    });

    REGISTER_FUNCTION("get_resource_count", [&](entt::entity stockpile, entt::entity resource) {
        return universe.get<components::ResourceStockpile>(stockpile)[resource];
    });
}

void FunctionCivilizations(cqsp::common::Universe& universe, ScriptInterface& script_engine) {
    CREATE_NAMESPACE(core);

    REGISTER_FUNCTION("get_player", [&]() { return universe.view<components::Player>().front(); });
}

void FunctionScience(cqsp::common::Universe& universe, ScriptInterface& script_engine) {
    CREATE_NAMESPACE(core);

    REGISTER_FUNCTION("create_lab", [&]() { return cqsp::common::systems::science::CreateLab(universe); });

    REGISTER_FUNCTION("add_science", [&](entt::entity lab, entt::entity research, double progress) {
        cqsp::common::systems::science::AddScienceResearch(universe, lab, research, progress);
    });

    REGISTER_FUNCTION("add_tech_progress", [&](entt::entity entity) {
        universe.emplace<components::science::TechnologicalProgress>(entity);
        universe.emplace<components::science::ScientificResearch>(entity);
    });

    REGISTER_FUNCTION("complete_technology", [&](entt::entity entity, entt::entity tech) {
        systems::science::ResearchTech(universe, entity, tech);
    });

    REGISTER_FUNCTION("research_technology", [&](entt::entity entity, entt::entity tech) {
        auto& res = universe.get<components::science::ScientificResearch>(entity);
        res.current_research[tech] = 0;
    });

    REGISTER_FUNCTION("add_potential_tech", [&](entt::entity entity, entt::entity tech) {
        auto& res = universe.get<components::science::ScientificResearch>(entity);
        res.potential_research.insert(tech);
    });
}

void LoadFunctions(Universe& universe, ScriptInterface& script_engine) {
    FunctionCivilizationGen(universe, script_engine);
    FunctionCivilizations(universe, script_engine);
    FunctionEconomy(universe, script_engine);
    FunctionPopulation(universe, script_engine);
    FunctionRandom(universe, script_engine);
    FunctionUniverseBodyGen(universe, script_engine);
    FunctionUser(universe, script_engine);
    FunctionEvent(universe, script_engine);
    FunctionShips(universe, script_engine);
    FunctionResource(universe, script_engine);
    FunctionScience(universe, script_engine);
}
}  // namespace


