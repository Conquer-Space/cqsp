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
#pragma once

#include <vector>

#include "common/systems/isimulationsystem.h"

namespace cqsp {
namespace common {
namespace systems {
class SysOrbit : public ISimulationSystem {
 public:
    explicit SysOrbit(Game& game) : ISimulationSystem(game) {}
    void DoSystem() override;
    int Interval() override { return 1; }

    void ParseOrbitTree(entt::entity parent, entt::entity body);
};

/// <summary>
/// Set's the SOI of the entity to the parent
/// </summary>
/// <param name="universe"></param>
/// <param name="body">Needs to have a Body and Orbit parameter</param>
void LeaveSOI(Universe& universe, const entt::entity& body);

class SysPath : public ISimulationSystem {
 public:
    explicit SysPath(Game& game) : ISimulationSystem(game) {}
    void DoSystem();
    int Interval() { return 1; }
};

class SysSurface : public ISimulationSystem {
 public:
    explicit SysSurface(Game& game) : ISimulationSystem(game) {}
    void DoSystem();
    int Interval() { return 1; }
};
}  // namespace systems
}  // namespace common
}  // namespace cqsp
