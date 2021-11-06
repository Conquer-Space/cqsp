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
#include "common/systems/sysinfrastructure.h"

#include "common/components/area.h"
#include "common/components/infrastructure.h"

void cqsp::common::systems::InfrastructureSim::DoSystem(Universe& universe) {
    namespace cqspc = cqsp::common::components;
    // Get all cities with industry and infrastruture
    auto view = universe.view<cqspc::Industry>();
    for (entt::entity entity : view) {
        auto& industry = universe.get<cqspc::Industry>(entity);
        double power_production = 0;
        double power_consumption = 0;
        for (entt::entity industrial_site : industry.industries) {
            if (universe.any_of<cqspc::infrastructure::PowerPlant>(industrial_site)) {
                power_production += universe.get<cqspc::infrastructure::PowerPlant>(industrial_site).production;
            }
            if (universe.any_of<cqspc::infrastructure::PowerConsumption>(industrial_site)) {
                power_consumption += universe.get<cqspc::infrastructure::PowerConsumption>(industrial_site).consumption;
            }
        }
        // Now assign infrastrutural information
        universe.emplace_or_replace<cqspc::infrastructure::CityPower>(entity, power_production, power_consumption);

        if (power_production < power_consumption) {
            // Then city has no power. Next time, we'd allow transmitting power, or allowing emergency use power
            // but for now, the city will go under brownout.
            universe.emplace_or_replace<cqspc::infrastructure::BrownOut>(entity);
        } else {
            universe.remove_if_exists<cqspc::infrastructure::BrownOut>(entity);
        }
    }
}
