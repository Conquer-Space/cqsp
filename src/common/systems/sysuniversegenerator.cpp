/* Conquer Space
* Copyright (C) 2021 Conquer Space
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
#include "common/systems/sysuniversegenerator.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>

#include "common/util/random/stdrandom.h"

#include "common/components/bodies.h"
#include "common/components/movement.h"
#include "common/components/organizations.h"
#include "common/components/player.h"
#include "common/components/surface.h"
#include "common/components/economy.h"
#include "common/components/name.h"
#include "common/components/population.h"
#include "common/components/area.h"
#include "common/components/resource.h"
#include "common/components/ships.h"

#include "common/systems/actions/factoryconstructaction.h"
#include "common/systems/economy/markethelpers.h"


void conquerspace::common::systems::universegenerator::ScriptUniverseGenerator::Generate(
    conquerspace::common::components::Universe& universe) {
    namespace cqspb = conquerspace::common::components::bodies;
    namespace cqsps = conquerspace::common::components::ships;
    namespace cqspt = conquerspace::common::components::types;
    namespace cqspc = conquerspace::common::components;

    script_engine["goods"] = universe.goods;
    script_engine["recipes"] = universe.recipes;

    // Create player
    auto player = universe.create();
    universe.emplace<cqspc::Organization>(player);
    universe.emplace<cqspc::Player>(player);

    for (int i = 0; i < 9; i++) {
        auto civ = universe.create();
        universe.emplace<cqspc::Organization>(civ);
    }

    sol::optional<sol::table> generator = script_engine["generators"]["data"][1];
    if (generator) {
        (*generator)["civ_init"]();
        script_engine["civilizations"] = sol::as_table(universe.view<cqspc::Organization>());
        (*generator)["universe_gen"]();
        auto view = universe.view<cqspc::Organization>();
        for (auto ent : view) {
            (*generator)["planets"](ent);
        }
    }
}
