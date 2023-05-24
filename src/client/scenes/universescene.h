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

#include <array>
#include <memory>
#include <utility>
#include <vector>

#include "client/systems/sysgui.h"
#include "client/systems/views/starsystemview.h"
#include "common/components/bodies.h"
#include "common/components/organizations.h"
#include "common/simulation.h"
#include "engine/application.h"
#include "engine/graphics/renderable.h"
#include "engine/renderer/renderer.h"
#include "engine/renderer/renderer2d.h"
#include "engine/scene.h"

namespace cqsp {
namespace scene {
class UniverseScene : public cqsp::engine::Scene {
 public:
    explicit UniverseScene(cqsp::engine::Application& app);
    ~UniverseScene() {
        // Delete ui
        simulation.reset();
        for (auto it = user_interfaces.begin(); it != user_interfaces.end(); it++) {
            it->reset();
        }
        for (auto& it : documents) {
            it.reset();
        }
        delete system_renderer;
    }

    void Init();
    void Update(float deltaTime);
    void Ui(float deltaTime);
    void Render(float deltaTime);

    template <class T>
    void AddUISystem() {
        auto ui = std::make_unique<T>(GetApp());
        ui->Init();
        user_interfaces.push_back(std::move(ui));
    }

    template <class T>
    void AddRmlUiSystem() {
        auto ui = std::make_unique<T>(GetApp());
        ui->OpenDocument();
        documents.push_back(std::move(ui));
    }

 private:
    /// <summary>
    /// Does the screenshot interface.
    /// </summary>
    void DoScreenshot();

    cqsp::engine::Renderable sphere;
    cqsp::engine::Renderable sky;
    cqsp::engine::Renderable planetDisp;
    cqsp::engine::Renderable sun;

    float x = 0, y = 0;

    double previous_mouseX;
    double previous_mouseY;

    entt::entity player;
    entt::entity selected_planet = entt::null;

    cqsp::client::systems::SysStarSystemRenderer* system_renderer;

    std::unique_ptr<cqsp::common::systems::simulation::Simulation> simulation;

    bool to_show_planet_window = false;

    // False is galaxy view, true is star system view
    bool view_mode = true;

    std::vector<std::unique_ptr<cqsp::client::systems::SysUserInterface>> user_interfaces;

    std::vector<std::unique_ptr<client::systems::SysRmlUiInterface>> documents;

    double last_tick = 0;

    std::array<int, 7> tick_speeds {1000, 500, 333, 100, 50, 10, 1};
    void ToggleTick();

    bool interp = true;
};

void SeePlanet(cqsp::engine::Application&, entt::entity);
entt::entity GetCurrentViewingPlanet(cqsp::common::Universe&);
// Halts all other things
void SetGameHalted(bool b);
bool IsGameHalted();
}  // namespace scene
}  // namespace cqsp
