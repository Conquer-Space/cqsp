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
#include "client/scenes/universescene.h"

#include <glad/glad.h>

#include <fmt/format.h>

#include <cmath>
#include <string>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/polar_coordinates.hpp>

#include "engine/renderer/primitives/uvsphere.h"
#include "engine/renderer/renderer.h"
#include "engine/renderer/primitives/cube.h"
#include "engine/renderer/primitives/polygon.h"
#include "engine/gui.h"

#include "common/components/bodies.h"
#include "common/components/organizations.h"
#include "common/components/player.h"
#include "common/components/coordinates.h"
#include "common/components/population.h"
#include "common/components/area.h"
#include "common/components/surface.h"
#include "common/components/name.h"
#include "common/components/resource.h"

#include "client/systems/sysplanetviewer.h"
#include "client/systems/systurnsavewindow.h"
#include "client/systems/sysstarsystemtree.h"
#include "client/systems/syspausemenu.h"
#include "client/systems/sysdebuggui.h"
#include "client/systems/syscommand.h"
#include "client/systems/gui/sysevent.h"

// If the game is paused or not, like when escape is pressed
bool game_halted = false;

cqsp::scene::UniverseScene::UniverseScene(cqsp::engine::Application& app) : Scene(app) {}

void cqsp::scene::UniverseScene::Init() {
    namespace cqspb = cqsp::common::components::bodies;
    namespace cqspco = cqsp::common;
    namespace cqspc = cqsp::common::components;
    namespace cqsps = cqsp::client::systems;

    using cqspco::systems::simulation::Simulation;
    simulation = std::make_unique<Simulation>(GetUniverse(), GetApp().GetScriptInterface());

    system_renderer = new cqsps::SysStarSystemRenderer(GetUniverse(), GetApp());
    system_renderer->Initialize();

    galaxy_renderer = new cqsps::GalaxyRenderer(GetUniverse(), GetApp());
    galaxy_renderer->Initialize();

    auto civilizationView = GetUniverse().view<cqspc::Civilization, cqspc::Player>();
    for (auto [entity, civ] : civilizationView.each()) {
        player = entity;
        player_civ = &civ;
    }
    cqspb::Body body = GetUniverse().get<cqspb::Body>(player_civ->starting_planet);
    system_renderer->SeeStarSystem(body.star_system);
    star_system = &GetUniverse().get<cqspb::StarSystem>(body.star_system);

    SeeStarSystem(GetApp(), body.star_system);
    SeePlanet(GetApp(), player_civ->starting_planet);

    selected_planet = player_civ->starting_planet;

    AddUISystem<cqsps::SysPlanetInformation>();
    AddUISystem<cqsps::SysTurnSaveWindow>();
    AddUISystem<cqsps::SysStarSystemTree>();
    AddUISystem<cqsps::SysPauseMenu>();
    AddUISystem<cqsps::SysDebugMenu>();
    AddUISystem<cqsps::SysCommand>();
    AddUISystem<cqsps::gui::SysEvent>();
    simulation->tick();
}

void cqsp::scene::UniverseScene::Update(float deltaTime) {
    if (!game_halted) {
        system_renderer->Update(deltaTime);
        galaxy_renderer->Update(deltaTime);
    }

    // Check for last tick
    if (GetUniverse().ToTick() && !game_halted) {
        // Game tick
        simulation->tick();
        system_renderer->OnTick();
        galaxy_renderer->OnTick();
    }

    GetUniverse().clear<cqsp::client::systems::MouseOverEntity>();
    system_renderer->GetMouseOnObject(GetApp().GetMouseX(), GetApp().GetMouseY());

    for (auto& ui : user_interfaces) {
        if (game_halted) {
            ui->window_flags = ImGuiWindowFlags_NoInputs;
        } else {
            ui->window_flags = 0;
        }
        ui->DoUpdate(deltaTime);
    }
}

void cqsp::scene::UniverseScene::Ui(float deltaTime) {
    for (auto& ui : user_interfaces) {
        ui->DoUI(deltaTime);
    }
    // Render star system renderer ui
    system_renderer->DoUI(deltaTime);
    galaxy_renderer->DoUI(deltaTime);
}

void cqsp::scene::UniverseScene::Render(float deltaTime) {
    glEnable(GL_MULTISAMPLE);
    //system_renderer->Render(deltaTime);
    galaxy_renderer->Render(deltaTime);
}

void cqsp::scene::SeeStarSystem(cqsp::engine::Application& app, entt::entity ent) {
    app.GetUniverse().clear<cqsp::client::systems::RenderingStarSystem>();
    app.GetUniverse().emplace<cqsp::client::systems::RenderingStarSystem>(ent);
}

entt::entity cqsp::scene::GetCurrentViewingPlanet(cqsp::engine::Application& app) {
    return app.GetUniverse().view<cqsp::client::systems::RenderingPlanet>().front();
}

void cqsp::scene::SeePlanet(cqsp::engine::Application& app, entt::entity ent) {
    app.GetUniverse().clear<cqsp::client::systems::RenderingPlanet>();
    app.GetUniverse().emplace<cqsp::client::systems::RenderingPlanet>(ent);
}

void cqsp::scene::SetGameHalted(bool b) { game_halted = b; }

bool cqsp::scene::IsGameHalted() { return game_halted; }
