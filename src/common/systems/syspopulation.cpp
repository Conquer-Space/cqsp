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
#include "syspopulation.h"

#include "common/components/population.h"
#include "common/components/resource.h"

void conquerspace::common::systems::SysPopulationGrowth::DoSystem(components::Universe& universe) {
    namespace cqspc = conquerspace::common::components;
    auto view = universe.view<cqspc::PopulationSegment>();
    for (auto [entity, segment] : view.each()) {
        float increase = static_cast<float>(Interval()) / 100000.f + 1;
        segment.population *= increase;
    }
}

void conquerspace::common::systems::SysPopulationConsumption::DoSystem(components::Universe& universe) {
    namespace cqspc = conquerspace::common::components;
    auto view = universe.view<cqspc::PopulationSegment>();
    for (auto [entity, segment] : view.each()) {
        // The population will feed, I guess
        entt::entity good = universe.goods["consumer_good"];
        long consumption = segment.population * 1;
        universe.get_or_emplace<cqspc::ResourceDemand>(entity)[good] = consumption;
    }
}
