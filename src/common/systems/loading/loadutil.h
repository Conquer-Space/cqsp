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

#include <hjson.h>
#include <string>
#include <map>

#include "common/universe.h"
#include "common/components/resource.h"

namespace cqsp::common::systems::loading {
/// <summary>
/// Returns true if name exists
/// </summary>
/// <returns></returns>
bool LoadName(Universe& universe, const entt::entity &entity, const Hjson::Value& value);
bool LoadIdentifier(Universe& universe, const entt::entity &entity, const Hjson::Value& value);
bool LoadDescription(Universe& universe, const entt::entity& entity, const Hjson::Value& value);

/// <summary>
/// Loads all the values that should be on every single data type
/// Returns true if an identifier exists
/// </summary>
bool LoadInitialValues(Universe& universe, const entt::entity& entity, const Hjson::Value& value);

components::ResourceLedger HjsonToLedger(cqsp::common::Universe&, Hjson::Value&);

bool VerifyHjsonValueExists(const Hjson::Value& value, const std::string& name, Hjson::Type type);
/// <summary>
/// For the values that *need* to exist
/// </summary>
/// <returns></returns>
bool VerifyInitialValues(const Hjson::Value& value, const std::map<std::string, Hjson::Type>& map);
}  // namespace cqsp::common::systems::loading
