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
#pragma once

#include "common/systems/isimulationsystem.h"
#include "common/universe.h"

namespace cqsp::common::systems {
// System for mines to reduce production so that production will stay stable if the price
// dips too low
// Main goal is to maintain stable pricing
class SysTrade : public ISimulationSystem {
 public:
    explicit SysTrade(Game& game) : ISimulationSystem(game) {}
    void DoSystem() override;
    int Interval() override { return components::StarDate::DAY; }
};
}  // namespace cqsp::common::systems
