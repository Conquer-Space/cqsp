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
#include "common/systems/scriptrunner.h"

#include <spdlog/spdlog.h>

#include <vector>
#include <string>

cqsp::common::systems::SysEventScriptRunner::SysEventScriptRunner(
    cqsp::common::Universe &_universe,
    scripting::ScriptInterface &interface) : universe(_universe), m_script_interface(interface) {
        sol::optional<std::vector<sol::table>> optional = m_script_interface["events"]["data"];
        events = *optional;
    // Add functions and stuff
}

void cqsp::common::systems::SysEventScriptRunner::ScriptEngine() {
    m_script_interface["date"] = universe.date.GetDate();
    for (auto &a : events) {
        sol::protected_function_result result = a["on_tick"](a);
        m_script_interface.ParseResult(result);
    }
}

cqsp::common::systems::SysEventScriptRunner::~SysEventScriptRunner() {
    // So it doesn't crash when we delete this
    for (auto& evet : events) {
        evet.abandon();
    }
    events.clear();
}

cqsp::common::systems::SysScript::~SysScript() {
    // So it doesn't crash when we delete this
    for (auto& evet : events) {
        evet.abandon();
    }
    events.clear();
}

void cqsp::common::systems::SysScript::DoSystem(Game &game) {
}
