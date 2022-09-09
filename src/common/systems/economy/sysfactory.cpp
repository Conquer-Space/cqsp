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
#include "common/systems/economy/sysfactory.h"

#include <spdlog/spdlog.h>

#include <tracy/Tracy.hpp>

#include "common/components/area.h"
#include "common/components/economy.h"
#include "common/components/infrastructure.h"
#include "common/util/profiler.h"
#include "common/components/surface.h"
#include "common/components/name.h"

namespace cqsp::common::systems {
namespace cqspc = cqsp::common::components;
namespace {
void ProcessIndustries(common::Universe& universe, entt::entity entity,
                       cqspc::Market& market) {
    // Get the transport cost
    auto& infrastructure =
        universe.get<cqspc::infrastructure::CityInfrastructure>(entity);
    // Calculate the infrastructure cost
    double infra_cost =
        infrastructure.default_purchase_cost - infrastructure.improvement;

    auto& industries = universe.get<cqspc::Industry>(entity);
    for (entt::entity productionentity : industries.industries) {
        // Process imdustries
        // Industries MUST have production and a linked recipe
        if (!universe.all_of<components::Production>(productionentity))
            continue;
        components::Recipe recipe = universe.get_or_emplace<components::Recipe>(
            universe.get<components::Production>(productionentity).recipe);
        components::FactorySize& size =
            universe.get_or_emplace<components::FactorySize>(productionentity,
                                                             1000.0);
        components::ProductionRatio& ratio =
            universe.get_or_emplace<components::ProductionRatio>(
                productionentity);

        components::ResourceLedger input =
            (recipe.input * ratio.input) +
            (recipe.capitalcost * (0.01 * size.size));
        // If there is not enough input goods, then restrict the trades
        // Input
        double input_transport_cost = input.GetSum() * infra_cost;

        components::ResourceLedger output = recipe.output * ratio.output;
        double output_transport_cost = output.GetSum() * infra_cost;

        // Get the number of items, and subtract from the wallet
        // Check supply if they can buy, or else they cannot boy
        if (market.previous_supply.HasAllResources(input)) {
            // Then they actually buy it
            market.demand += input;
        } else {
            // Then they cannot produce for the next round
            // Reset the capital costs
            components::CostBreakdown& costs =
                universe.get_or_emplace<components::CostBreakdown>(
                    productionentity);
            costs.Reset();
            continue;
        }

        // Check demand if there is demand
        if (market.previous_demand.HasAllResources(output)) {
            // Then they can sell it
            market.supply += output;
        } else {
            // IDK what to do
        }

        // Next time need to compute the costs along with input and output so that the
        // factory doesn't overspend. We sorta need a balanced economy
        components::CostBreakdown& costs =
            universe.get_or_emplace<components::CostBreakdown>(productionentity);

        // Maintainence costs will still have to be upkept, so if there isnt any resources
        // to upkeep the place, then stop the production
        costs.maintenance =
            (recipe.capitalcost * market.price).GetSum() * 0.01 * size.size;
        costs.materialcosts =
            (recipe.input * ratio.input * market.price).GetSum();
        costs.profit = (recipe.output * ratio.output * market.price).GetSum();
        costs.wages = size.size * 1000 * 50000;
        costs.net = costs.profit - costs.maintenance - costs.materialcosts -
                    costs.wages;
        costs.transport = output_transport_cost + input_transport_cost;
        if (costs.net > 0) {
            size.size *= 1.02;
        } else {
            size.size *= 0.99;
        }

        ratio.input = recipe.input.UnitLeger(size.size);
        ratio.output = recipe.output.UnitLeger(size.size);
    }
}
}  // namespace

void SysProduction::DoSystem() {
    ZoneScoped;
    Universe& universe = GetUniverse();
    auto view = universe.view<components::Industry>();
    BEGIN_TIMED_BLOCK(INDUSTRY);
    int factories = 0;
// Loop through the markets
    auto market_view = universe.view<cqspc::Habitation> ();
    int settlement_count = 0;
    for (entt::entity entity : market_view) {
        auto& market = universe.get_or_emplace<cqspc::Market>(entity);
        // Read the segment information
        // Get the children of the market
        auto& habitation = universe.get<cqspc::Habitation>(entity);
        for (entt::entity settlement : habitation.settlements) {
            ProcessIndustries(universe, settlement, market);
        }
    }
    END_TIMED_BLOCK(INDUSTRY);
    SPDLOG_TRACE("Updated {} factories, {} industries", factories, view.size());
}
}  // namespace cqsp::common::systems
