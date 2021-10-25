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
#include "client/systems/sysstarsystemrenderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <noise/noise.h>

#include <cmath>
#include <string>
#include <memory>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/polar_coordinates.hpp>
#include <glm/gtx/string_cast.hpp>

#include "client/components/planetrendering.h"

#include "engine/renderer/primitives/uvsphere.h"
#include "engine/renderer/renderer.h"
#include "engine/renderer/primitives/cube.h"
#include "engine/renderer/primitives/polygon.h"
#include "engine/renderer/primitives/pane.h"

#include "common/components/bodies.h"
#include "common/components/surface.h"
#include "common/components/organizations.h"
#include "common/components/player.h"
#include "common/components/resource.h"
#include "common/components/coordinates.h"
#include "common/components/name.h"
#include "common/components/ships.h"
#include "common/components/units.h"
#include "common/util/profiler.h"

using cqsp::client::systems::SysStarSystemRenderer;
SysStarSystemRenderer::SysStarSystemRenderer(cqsp::common::Universe &_u,
                                                cqsp::engine::Application &_a) :
                                                m_universe(_u), m_app(_a),
                                                scroll(5), view_x(0),
                                                view_y(0), view_center(glm::vec3(1, 1, 1)),
                                                sun_color(glm::vec3(10, 10, 10)) {
}

struct Offset  {
    glm::vec3 offset;
};

void SysStarSystemRenderer::Initialize() {
    // Initialize meshes, etc
    cqsp::engine::Mesh* sphere_mesh = new cqsp::engine::Mesh();
    cqsp::primitive::ConstructSphereMesh(64, 64, *sphere_mesh);

    // Initialize sky box
    asset::Texture* sky_texture = m_app.GetAssetManager()
        .GetAsset<cqsp::asset::Texture>("core:skycubemap");

    asset::ShaderProgram_t skybox_shader = m_app.GetAssetManager().MakeShader("core:skycubevert", "core:skycubefrag");

    sky.mesh = new cqsp::engine::Mesh();
    primitive::MakeCube(*sky.mesh);
    sky.shaderProgram = skybox_shader;
    sky.SetTexture("texture0", 0, sky_texture);

    asset::ShaderProgram_t circle_shader = m_app.GetAssetManager().
                                            MakeShader("core:shader.pane.vert", "core:coloredcirclefrag");

    planet_circle.mesh = new cqsp::engine::Mesh();
    primitive::CreateFilledCircle(*planet_circle.mesh);
    planet_circle.shaderProgram = circle_shader;

    ship_overlay.mesh = new cqsp::engine::Mesh();
    primitive::CreateFilledTriangle(*ship_overlay.mesh);
    ship_overlay.shaderProgram = circle_shader;

    city.mesh = new cqsp::engine::Mesh();
    primitive::CreateFilledSquare(*city.mesh);
    city.shaderProgram = circle_shader;

    // Initialize shaders
    asset::ShaderProgram_t planet_shader = m_app.GetAssetManager().MakeShader("core:objectvert", "core:planetfrag");
    pbr_shader = planet_shader;

    no_light_shader = m_app.GetAssetManager().MakeShader("core:objectvert", "core:skyboxfrag");

    // Planet spheres
    planet.mesh = sphere_mesh;
    planet_shader->UseProgram();
    planet_shader->setInt("albedomap", 0);
    planet_shader->setInt("heightmap", 1);

    planet.shaderProgram = planet_shader;

    // Initialize sun
    sun.mesh = sphere_mesh;
    sun.shaderProgram = m_app.GetAssetManager().MakeShader("core:objectvert", "core:sunshader");

    auto buffer_shader = m_app.GetAssetManager().MakeShader("core:framebuffervert", "core:framebufferfrag");
    ship_icon_layer = renderer.AddLayer<engine::AAFrameBufferRenderer>(buffer_shader, *m_app.GetWindow());
    physical_layer = renderer.AddLayer<engine::AAFrameBufferRenderer>(buffer_shader, *m_app.GetWindow());
    planet_icon_layer = renderer.AddLayer<engine::AAFrameBufferRenderer>(buffer_shader, *m_app.GetWindow());
    skybox_layer = renderer.AddLayer<engine::AAFrameBufferRenderer>(buffer_shader, *m_app.GetWindow());
}

void SysStarSystemRenderer::OnTick() {
    entt::entity current_planet = m_app.GetUniverse().view<RenderingPlanet>().front();
    if (current_planet != entt::null) {
        view_center = CalculateObjectPos(m_viewing_entity);
    }

    namespace cqspb = cqsp::common::components::bodies;

    entt::entity system = m_app.GetUniverse().view<RenderingStarSystem>().front();
    auto &system_comp = m_app.GetUniverse().get<cqspb::StarSystem>(system);
    for (entt::entity ent : system_comp.bodies) {
        m_app.GetUniverse().emplace_or_replace<ToRender>(ent);
    }
}

void SysStarSystemRenderer::Render() {
    namespace cqspb = cqsp::common::components::bodies;
    // Check for resized window
    window_ratio = static_cast<float>(m_app.GetWindowWidth()) /
                   static_cast<float>(m_app.GetWindowHeight());
    renderer.NewFrame(*m_app.GetWindow());

    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    entt::entity current_planet = m_app.GetUniverse().view<RenderingPlanet>().front();
    if (current_planet != m_viewing_entity) {
        SPDLOG_INFO("Switched displaying planet, seeing {}", current_planet);
        m_viewing_entity = current_planet;
        // Do terrain
        SeeEntity();
    }

    m_star_system  = m_app.GetUniverse().view<RenderingStarSystem>().front();

    CalculateCamera();

    if (second_terrain_complete && !terrain_complete) {
        SPDLOG_INFO("Done less detailed planet generation");
        less_detailed_terrain_generator_thread.join();
        SetPlanetTexture(intermediate_image_generator);
        second_terrain_complete = false;
    }

    if (terrain_complete) {
        SPDLOG_INFO("Done terrain generation");
        terrain_generator_thread.join();
        SetPlanetTexture(final_image_generator);
        terrain_complete = false;
    }

    DrawStars();
    DrawBodies();
    DrawShips();
    DrawSkybox();

    renderer.DrawAllLayers();
}

void SysStarSystemRenderer::SeeStarSystem(entt::entity system) {
    namespace cqspb = cqsp::common::components::bodies;
    if (m_star_system != entt::null &&
        m_universe.all_of<cqspb::StarSystem>(m_star_system)) {
        // Remove tags
        auto star_system_component = m_universe.get<cqspb::StarSystem>(m_star_system);
        for (auto body : star_system_component.bodies) {
            // Add a tag
            m_universe.remove_if_exists<ToRender>(body);
        }
    }

    m_star_system = system;
    auto star_system_component = m_universe.get<cqspb::StarSystem>(m_star_system);
    for (auto body : star_system_component.bodies) {
        // Add a tag
        m_universe.emplace_or_replace<ToRender>(body);
    }
}

void SysStarSystemRenderer::SeeEntity() {
    namespace cqspb = cqsp::common::components::bodies;

    // See the object
    view_center = CalculateObjectPos(m_viewing_entity);

    CalculateCityPositions();
    // If it has a terrain, then do things, if it doesn't have a terrain, render a blank sphere
    int seed = 0;
    if (!m_app.GetUniverse().all_of<cqspb::Terrain>(m_viewing_entity)) {
        return;
    }

    // Set seed
    auto &terrain = m_app.GetUniverse().get<cqspb::Terrain>(m_viewing_entity);
    cqsp::client::systems::TerrainImageGenerator generator;
    generator.terrain = terrain;
    generator.GenerateTerrain(m_universe, 1, 2);
    SetPlanetTexture(generator);

    // TODO(EhWhoAmI):  If it's in the process of generating, find some way to kill the gen,
    // and move on to the new terrain.
    SPDLOG_INFO("Generating terrain");
    // Generate terrain
    intermediate_image_generator.terrain = terrain;
    final_image_generator.terrain = terrain;

    if (less_detailed_terrain_generator_thread.joinable()) {
        less_detailed_terrain_generator_thread.join();
    }
    if (terrain_generator_thread.joinable()) {
        terrain_generator_thread.join();
    }

    less_detailed_terrain_generator_thread = std::thread([&]() {
        // Generate slightly less detailed terrain so that it looks better at first
        intermediate_image_generator.GenerateTerrain(m_universe, 4, 6);
        second_terrain_complete = true;
    });

    terrain_generator_thread = std::thread([&]() {
        // Generate slightly less detailed terrain so that it looks better at first
        auto start = std::chrono::high_resolution_clock::now();
        final_image_generator.GenerateTerrain(m_universe, 5, 10);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        terrain_complete = true;
    });
}

void SysStarSystemRenderer::Update() {
    double deltaX = previous_mouseX - m_app.GetMouseX();
    double deltaY = previous_mouseY - m_app.GetMouseY();
    if (!ImGui::GetIO().WantCaptureMouse) {
        if (scroll + m_app.GetScrollAmount() * 3 > 1.5) {
            scroll += m_app.GetScrollAmount() * 3;
        }

        if (m_app.MouseButtonIsHeld(GLFW_MOUSE_BUTTON_LEFT)) {
            view_x += deltaX/m_app.GetWindowWidth()*3.1415*4;
            view_y -= deltaY/m_app.GetWindowHeight()*3.1415*4;

            if (glm::degrees(view_y) > 89.f) {
                view_y = glm::radians(89.f);
            }
            if (glm::degrees(view_y) < -89.f) {
                view_y = glm::radians(-89.f);
            }
        }

        previous_mouseX = m_app.GetMouseX();
        previous_mouseY = m_app.GetMouseY();

        // If clicks on object, go to the planet
        entt::entity ent = m_app.GetUniverse().view<MouseOverEntity>().front();
        if (m_app.MouseButtonIsReleased(GLFW_MOUSE_BUTTON_LEFT) && ent != entt::null && !m_app.MouseDragged()) {
            // Then go to the object
            SeePlanet(ent);
        }
    }
    // Check if it has terrain resource rendering, and make terrain thing
    if (m_viewing_entity != entt::null &&
        m_app.GetUniverse().all_of<cqsp::client::components::PlanetTerrainRender>(m_viewing_entity)) {
        // Then check if it's the same rendered object
        auto &rend = m_app.GetUniverse().get<cqsp::client::components::PlanetTerrainRender>(m_viewing_entity);
        if (rend.resource != terrain_displaying) {
            // Check if it's the same
            using cqsp::common::components::ResourceDistribution;
            if (m_app.GetUniverse().any_of<ResourceDistribution>(m_viewing_entity)) {
                auto &dist = m_app.GetUniverse().get<ResourceDistribution>(m_viewing_entity);
                TerrainImageGenerator gen;
                // Hack to do this, will probably have to rework this in the future.
                cqsp::common::components::bodies::Terrain t;
                t.seed = dist[rend.resource];
                gen.terrain = t;

                gen.GenerateHeightMap(3, 9);
                // Make the UI
                unsigned int a = GeneratePlanetTexture(gen.GetHeightMap());
                planet_resource = GenerateTexture(a, gen.GetHeightMap());
                terrain_displaying = rend.resource;
                // Switch view mode
                planet.textures[0] = planet_resource;
                planet.shaderProgram = no_light_shader;
            }
        }
    } else if (m_viewing_entity != entt::null) {
        // Reset to default
        planet.textures[0] = planet_texture;
        terrain_displaying = entt::null;
        // Also change up the shader
        planet.shaderProgram = pbr_shader;
    }
}

void SysStarSystemRenderer::SeePlanet(entt::entity ent) {
    m_app.GetUniverse().clear<RenderingPlanet>();
    m_app.GetUniverse().emplace<RenderingPlanet>(ent);
}

void SysStarSystemRenderer::DoUI() {
    // UI for debug in the future.
}

void SysStarSystemRenderer::DrawStars() {
    // Draw stars
    namespace cqspb = cqsp::common::components::bodies;
    namespace cqsps = cqsp::common::components::ships;
    auto stars = m_app.GetUniverse().view<ToRender, cqspb::Body, cqspb::LightEmitter>();
    for (auto ent_id : stars) {
        // Draw the star circle
        glm::vec3 object_pos = CalculateCenteredObject(ent_id);
        sun_position = object_pos;
        if (glm::distance(object_pos, cam_pos) > 900) {
            // Check if it's obscured by a planet, but eh, we can deal with it later
            planet_circle.shaderProgram->UseProgram();
            planet_circle.shaderProgram->setVec4("color", 1, 1, 0, 1);
            DrawPlanetIcon(object_pos);
            continue;
        }
        renderer.BeginDraw(physical_layer);
        DrawStar(object_pos);
        renderer.EndDraw(physical_layer);
    }
}

void cqsp::client::systems::SysStarSystemRenderer::DrawBodies() {
    namespace cqspb = cqsp::common::components::bodies;
    namespace cqsps = cqsp::common::components::ships;
    // Draw other bodies
    auto bodies = m_app.GetUniverse().view<ToRender, cqspb::Body>(entt::exclude<cqspb::LightEmitter>);
    for (auto body_entity : bodies) {
        // Draw the planet circle
        glm::vec3 object_pos = CalculateCenteredObject(body_entity);

        // Draw Ships
        namespace cqspc = cqsp::common::components;
        namespace cqspt = cqsp::common::components::types;

        if (glm::distance(object_pos, cam_pos) > 200) {
            // Check if it's obscured by a planet, but eh, we can deal with it later
            // Set planet circle color
            planet_circle.shaderProgram->UseProgram();
            planet_circle.shaderProgram->setVec4("color", 0, 0, 1, 1);
            renderer.BeginDraw(planet_icon_layer);
            DrawEntityName(object_pos, body_entity);
            DrawPlanetIcon(object_pos);
            renderer.EndDraw(planet_icon_layer);
            continue;
        } else {
            // Check if planet has terrain or not
            renderer.BeginDraw(physical_layer);
            if (m_app.GetUniverse().all_of<cqspb::Terrain>(m_viewing_entity)) {
                // Do empty terrain
                DrawPlanet(m_app.GetUniverse().get<cqspb::Terrain>(m_viewing_entity).terrain_type, object_pos);
            } else {
                DrawTerrainlessPlanet(object_pos);
            }
            renderer.EndDraw(physical_layer);
            RenderCities(object_pos, body_entity);
        }
    }
}

void SysStarSystemRenderer::DrawShips() {
    namespace cqsps = cqsp::common::components::ships;
    // Draw Ships
    auto ships = m_app.GetUniverse().view<ToRender, cqsps::Ship>();

    renderer.BeginDraw(ship_icon_layer);
    ship_overlay.shaderProgram->UseProgram();
    for (auto ent_id : ships) {
        glm::vec3 object_pos = CalculateCenteredObject(ent_id);
        ship_overlay.shaderProgram->setVec4("color", 1, 0, 0, 1);
        DrawShipIcon(object_pos);
    }
    renderer.EndDraw(ship_icon_layer);
}

void SysStarSystemRenderer::DrawSkybox() {
    // Draw sky box
    renderer.BeginDraw(skybox_layer);
    sky.shaderProgram->UseProgram();
    sky.shaderProgram->setMat4("view", glm::mat4(glm::mat3(camera_matrix)));
    sky.shaderProgram->setMat4("projection", projection);
    glDepthFunc(GL_LEQUAL);
    // skybox cube
    engine::Draw(sky);
    glDepthFunc(GL_LESS);
    renderer.EndDraw(skybox_layer);
}

void SysStarSystemRenderer::DrawEntityName(glm::vec3 &object_pos,
                                           entt::entity ent_id) {
    using cqsp::common::components::Name;
    std::string text = "";
    if (m_app.GetUniverse().all_of<Name>(ent_id)) {
        text = m_app.GetUniverse().get<Name>(ent_id);
    } else {
        text = fmt::format("{}", ent_id);
    }
    glm::vec3 pos = glm::project(object_pos, camera_matrix, projection, viewport);
    // Check if the position on screen is within bounds
    if (!(pos.z >= 1 || pos.z <= -1) &&
        (pos.x > 0 && pos.x < m_app.GetWindowWidth() &&
            pos.y > 0 && pos.y < m_app.GetWindowHeight())) {
        m_app.DrawText(text, pos.x, pos.y);
    }
}

void SysStarSystemRenderer::DrawPlanetIcon(glm::vec3 &object_pos) {
    glm::vec3 pos = glm::project(object_pos, camera_matrix, projection, viewport);
    glm::mat4 planetDispMat = glm::mat4(1.0f);
    if (pos.z >= 1 || pos.z <= -1) {
        return;
    }

    planetDispMat = glm::translate(planetDispMat, TranslateToNormalized(pos));
    planetDispMat = glm::scale(planetDispMat, glm::vec3(circle_size, circle_size, circle_size));

    float window_ratio = GetWindowRatio();
    planetDispMat = glm::scale(planetDispMat, glm::vec3(1, window_ratio, 1));
    glm::mat4 twodimproj =  glm::mat4(1.0f);

    planet_circle.shaderProgram->UseProgram();
    planet_circle.shaderProgram->setMat4("model", planetDispMat);
    planet_circle.shaderProgram->setMat4("projection", twodimproj);

    engine::Draw(planet_circle);
}

void SysStarSystemRenderer::DrawCityIcon(glm::vec3 &object_pos) {
    glm::vec3 pos = glm::project(object_pos, camera_matrix, projection, viewport);
    glm::mat4 planetDispMat = glm::mat4(1.0f);
    if (pos.z >= 1 || pos.z <= -1) {
        return;
    }

    planetDispMat = glm::translate(planetDispMat, TranslateToNormalized(pos));

    planetDispMat = glm::scale(planetDispMat, glm::vec3(circle_size, circle_size, circle_size));

    float window_ratio = GetWindowRatio();
    planetDispMat = glm::scale(planetDispMat, glm::vec3(1, window_ratio, 1));
    glm::mat4 twodimproj = glm::mat4(1.0f);
    city.shaderProgram->UseProgram();
    city.shaderProgram->setMat4("model", planetDispMat);
    city.shaderProgram->setMat4("projection", twodimproj);

    engine::Draw(city);
}

void SysStarSystemRenderer::DrawShipIcon(glm::vec3 &object_pos) {
    glm::vec3 pos = glm::project(object_pos, camera_matrix, projection, viewport);
    glm::mat4 shipDispMat = glm::mat4(1.0f);
    if (pos.z >= 1 || pos.z <= -1) {
        return;
    }

    shipDispMat = glm::translate(shipDispMat, TranslateToNormalized(pos));

    shipDispMat = glm::scale(shipDispMat, glm::vec3(circle_size, circle_size, circle_size));

    float window_ratio = GetWindowRatio();
    shipDispMat = glm::scale(shipDispMat, glm::vec3(1, window_ratio, 1));
    glm::mat4 twodimproj = glm::mat4(1.0f);
    ship_overlay.shaderProgram->UseProgram();
    ship_overlay.shaderProgram->setMat4("model", shipDispMat);
    ship_overlay.shaderProgram->setMat4("projection", twodimproj);

    engine::Draw(ship_overlay);
}

void SysStarSystemRenderer::DrawPlanet(entt::entity terrain, glm::vec3 &object_pos) {
    glm::mat4 position = glm::mat4(1.f);
    position = glm::translate(position, object_pos);

    glm::mat4 transform = glm::mat4(1.f);
    // For some reason, the sphere we make needs to be inverted
    transform = glm::scale(transform, glm::vec3(1, 1, 1));

    position = position * transform;

    planet.SetMVP(position, camera_matrix, projection);
    planet.shaderProgram->UseProgram();

    planet.shaderProgram->setVec3("lightDir", glm::normalize(sun_position - object_pos));
    planet.shaderProgram->setVec3("lightPosition", sun_position);

    planet.shaderProgram->setVec3("lightColor", sun_color);
    planet.shaderProgram->setVec3("viewPos", cam_pos);

    using cqsp::common::components::bodies::TerrainData;
    planet.shaderProgram->Set("seaLevel", m_universe.get<TerrainData>(terrain).sea_level);
    engine::Draw(planet);
}

void SysStarSystemRenderer::DrawStar(glm::vec3 &object_pos) {
    glm::mat4 position = glm::mat4(1.f);
    position = glm::translate(position, object_pos);

    glm::mat4 transform = glm::mat4(1.f);
    transform = glm::scale(transform, glm::vec3(5, 5, 5));
    position = position * transform;

    sun.SetMVP(position, camera_matrix, projection);
    sun.shaderProgram->setVec4("color", 1, 1, 1, 1);
    engine::Draw(sun);
}

void SysStarSystemRenderer::DrawTerrainlessPlanet(glm::vec3 &object_pos) {
    glm::mat4 position = glm::mat4(1.f);
    position = glm::translate(position, object_pos);

    glm::mat4 transform = glm::mat4(1.f);
    position = position * transform;

    sun.SetMVP(position, camera_matrix, projection);
    sun.shaderProgram->setVec4("color", 1, 0, 1, 1);
    engine::Draw(sun);
}

void SysStarSystemRenderer::RenderCities(glm::vec3 &object_pos,
                                        const entt::entity &body_entity) {
    // Draw Ships
    namespace cqspc = cqsp::common::components;
    namespace cqspt = cqsp::common::components::types;
    if (!m_app.GetUniverse().all_of<cqspc::Habitation>(body_entity)) {
        return;
    }
    std::vector<entt::entity> cities = m_app.GetUniverse().get<cqspc::Habitation>(body_entity).settlements;
    if (cities.empty()) {
        return;
    }

    renderer.BeginDraw(ship_icon_layer);
    city.shaderProgram->UseProgram();
    city.shaderProgram->setVec4("color", 0.5, 0.5, 0.5, 1);
    for (auto city_entity : cities) {
        // Calculate position to render
        glm::vec3 city_pos = m_app.GetUniverse().get<Offset>(city_entity).offset;
        if (glm::length(city_pos - cam_pos) < (scroll - 0.2)) {
            // If it's reasonably close, then we can show city names
            glm::vec3 pos = city_pos + object_pos;
            if (scroll < 3) {
                DrawEntityName(pos, city_entity);
            }
            DrawCityIcon(pos);
        }
    }
    renderer.EndDraw(ship_icon_layer);
}

void SysStarSystemRenderer::CalculateCityPositions() {
    namespace cqspc = cqsp::common::components;
    namespace cqspt = cqsp::common::components::types;
    // Calculate offset for all cities on planet if they exist
    if (!m_app.GetUniverse().all_of<cqspc::Habitation>(m_viewing_entity)) {
        return;
    }
    std::vector<entt::entity> cities = m_app.GetUniverse().get<cqspc::Habitation>(m_viewing_entity).settlements;
    if (cities.empty()) {
        return;
    }
    for (auto city_entity : cities) {
        if (!m_app.GetUniverse().all_of<cqspt::SurfaceCoordinate>(city_entity)) {
            continue;
        }
        auto& coord = m_app.GetUniverse().get<cqspt::SurfaceCoordinate>(city_entity);
        m_app.GetUniverse().emplace_or_replace<Offset>(city_entity, cqspt::toVec3(coord,  1));
    }
    SPDLOG_INFO("Calculated offset");
}

glm::vec3 SysStarSystemRenderer::CalculateObjectPos(const entt::entity &ent) {
    //namespace cqspb = cqsp::common::components::bodies;
    namespace cqspt = cqsp::common::components::types;
    // Get the things
    if (m_universe.all_of<cqspt::Kinematics>(ent)) {
        return (m_universe.get<cqspt::Kinematics>(ent).position / 0.01f);
    }
    return glm::vec3(0, 0, 0);
}

glm::vec3 SysStarSystemRenderer::CalculateCenteredObject(const glm::vec3 &vec) {
    return vec - view_center;
}

glm::vec3 SysStarSystemRenderer::TranslateToNormalized(const glm::vec3 &pos) {
    return glm::vec3((pos.x / m_app.GetWindowWidth() - 0.5) * 2,
            (pos.y / m_app.GetWindowHeight() - 0.5) * 2, 0);
}

glm::vec3 SysStarSystemRenderer::CalculateCenteredObject(const entt::entity &ent) {
    return CalculateCenteredObject(CalculateObjectPos(ent));
}

void SysStarSystemRenderer::CalculateCamera() {
    cam_pos = glm::vec3(
                cos(view_y) * sin(view_x),
                sin(view_y),
                cos(view_y) * cos(view_x)) * scroll;
    cam_up = glm::vec3(0.0f, 1.0f, 0.0f);
    camera_matrix = glm::lookAt(cam_pos, glm::vec3(0.f, 0.f, 0.f), cam_up);
    projection = glm::infinitePerspective(glm::radians(45.f), GetWindowRatio(), 0.1f);
    viewport = glm::vec4(0.f, 0.f, m_app.GetWindowWidth(), m_app.GetWindowHeight());
}

void SysStarSystemRenderer::SetPlanetTexture(TerrainImageGenerator &generator) {
    SPDLOG_INFO("Set terrain");
    unsigned int gl_planet_texture = GeneratePlanetTexture(generator.GetAlbedoMap());
    unsigned int gl_planet_heightmap  = GeneratePlanetTexture(generator.GetHeightMap());
    generator.ClearData();

    // Free textures in texture
    for (auto t : planet.textures) {
        delete t;
    }
    planet.textures.clear();

    planet_texture = GenerateTexture(gl_planet_texture, generator.GetAlbedoMap());
    planet_heightmap = GenerateTexture(gl_planet_heightmap, generator.GetAlbedoMap());

    // Assign textures
    planet.textures.push_back(planet_texture);
    planet.textures.push_back(planet_heightmap);
}

unsigned int SysStarSystemRenderer::GeneratePlanetTexture(noise::utils::Image &image) {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.GetWidth(), image.GetHeight(), 0, GL_RGBA,
                          GL_UNSIGNED_INT_8_8_8_8, image.GetConstSlabPtr());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texture;
}

glm::vec3 SysStarSystemRenderer::CalculateMouseRay(const glm::vec3 &ray_nds) {
    glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, -1.0, 1.0);
    glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);
    glm::vec4 inv = (glm::inverse(camera_matrix) * ray_eye);

    // Normalize vector
    return glm::normalize(glm::vec3(inv.x, inv.y, inv.z));
}

float SysStarSystemRenderer::GetWindowRatio() {
    return window_ratio;
}

cqsp::asset::Texture* SysStarSystemRenderer::GenerateTexture(
    unsigned int tex, noise::utils::Image &image) {
    cqsp::asset::Texture *texture = new cqsp::asset::Texture();
    texture->id = tex;
    texture->width = image.GetWidth();
    texture->height = image.GetHeight();
    texture->texture_type = GL_TEXTURE_2D;
    return texture;
}

entt::entity SysStarSystemRenderer::GetMouseOnObject(int mouse_x, int mouse_y) {
    namespace cqspb = cqsp::common::components::bodies;

    // Loop through objects
    auto bodies = m_app.GetUniverse().view<ToRender, cqspb::Body>();
    for (entt::entity ent_id : bodies) {
        glm::vec3 object_pos = CalculateCenteredObject(ent_id);
        // Check if the sphere is rendered or not
        if (glm::distance(object_pos, cam_pos) > 100) {
            // Calculate circle
            glm::vec3 pos = glm::project(object_pos, camera_matrix, projection, viewport);
            if (pos.z >= 1) {
                continue;
            }

            // Check if it's intersecting
            float dim = circle_size * m_app.GetWindowHeight();
            if (glm::distance(glm::vec2(pos.x, m_app.GetWindowHeight() - pos.y),
                    glm::vec2(mouse_x, mouse_y)) <= dim) {
                m_app.GetUniverse().emplace<MouseOverEntity>(ent_id);
                return ent_id;
            }
        } else {
            // Normalize 3d device coordinates
            float x = (2.0f * mouse_x) / m_app.GetWindowWidth() - 1.0f;
            float y = 1.0f - (2.0f * mouse_y) / m_app.GetWindowHeight();
            float z = 1.0f;

            glm::vec3 ray_wor = CalculateMouseRay(glm::vec3(x, y, z));

            float radius = 1;
            if (m_app.GetUniverse().all_of<cqspb::LightEmitter>(ent_id)) {
                radius = 10;
            }

            // Check for intersection for sphere
            glm::vec3 sub = cam_pos - object_pos;
            float b = glm::dot(ray_wor, sub);
            float c = glm::dot(sub, sub) - radius * radius;
            if ((b * b - c) >= 0) {
                m_app.GetUniverse().emplace<MouseOverEntity>(ent_id);
                return ent_id;
            }
        }
    }
    return entt::null;
}

SysStarSystemRenderer::~SysStarSystemRenderer() {
    if (less_detailed_terrain_generator_thread.joinable()) {
        less_detailed_terrain_generator_thread.join();
    }
    if (terrain_generator_thread.joinable()) {
        terrain_generator_thread.join();
    }
}
