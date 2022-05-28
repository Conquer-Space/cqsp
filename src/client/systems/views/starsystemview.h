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

#include <map>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "common/universe.h"
#include "engine/graphics/renderable.h"
#include "engine/renderer/framebuffer.h"
#include "engine/renderer/renderer.h"
#include "engine/application.h"
#include "common/components/coordinates.h"

namespace cqsp {
namespace client {
namespace systems {
// TODO(EhWhoAmI): Would be helpful to move the following structs to a header file.
/*
 * Tag class for bodies to render.
 */
struct ToRender {};

struct MouseOverEntity {};

// Planet that the camera center is at
struct FocusedPlanet {};
// City to look at
struct FocusedCity {};

struct CityFounding {};

/*
 * Main renderer for the universe
 */
class SysStarSystemRenderer {
 public:
    SysStarSystemRenderer(cqsp::common::Universe &,
                          cqsp::engine::Application &);
    void Initialize();
    void OnTick();
    void Render(float deltaTime);
    void SeeStarSystem();
    void SeeEntity();
    void Update(float deltaTime);
    void SeePlanet(entt::entity);
    void DoUI(float deltaTime);

    glm::vec3 GetMouseIntersectionOnObject(int mouse_x, int mouse_y);

    // The angle the camera is looking from
    float view_x;
    // The angle the camera is looking away from
    float view_y;

    double previous_mouseX;
    double previous_mouseY;

    double scroll = 10;
    double min_zoom = 1;
    // Light year sized
    double max_zoom = 9.4605284e15;

    glm::vec3 view_center;

    double GetDivider() { return divider; }

    entt::entity GetMouseOnObject(int mouse_x, int mouse_y);

    static bool IsFoundingCity(common::Universe& universe);

    void DrawOrbit(const entt::entity& entity);

    ~SysStarSystemRenderer();

 private:
    entt::entity m_viewing_entity = entt::null;
    entt::entity terrain_displaying = entt::null;

    cqsp::common::Universe &m_universe;
    cqsp::engine::Application &m_app;

    cqsp::engine::Renderable planet;
    cqsp::engine::Renderable textured_planet;
    cqsp::engine::Renderable sky;
    cqsp::engine::Renderable planet_circle;
    cqsp::engine::Renderable ship_overlay;
    cqsp::engine::Renderable city;
    cqsp::engine::Renderable sun;

    cqsp::asset::ShaderProgram_t orbit_shader;
#if FALSE
    // Disabled for now
    asset::ShaderProgram_t no_light_shader;
#endif

    cqsp::asset::Texture* planet_texture;
    cqsp::asset::Texture* planet_heightmap;
    cqsp::asset::Texture* planet_resource;

    glm::vec3 cam_pos;
    glm::vec3 cam_up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 camera_matrix;
    glm::mat4 projection;
    glm::vec4 viewport;

    float circle_size = 0.01f;

    void DrawStars();
    void DrawBodies();
    void DrawShips();
    void DrawSkybox();

    void DrawEntityName(glm::vec3 &object_pos, entt::entity ent_id);
    void DrawPlanetIcon(glm::vec3 &object_pos);
    void DrawShipIcon(glm::vec3 &object_pos);
    void DrawCityIcon(glm::vec3 &object_pos);
    void DrawPlanet(glm::vec3 &object_pos, entt::entity entity);
    void DrawTexturedPlanet(glm::vec3 &object_pos, entt::entity entity);
    void DrawStar(const entt::entity& entity, glm::vec3 &object_pos);
    void DrawTerrainlessPlanet(glm::vec3 &object_pos);
    void RenderCities(glm::vec3 &object_pos, const entt::entity &body_entity);
    bool CityIsVisible(glm::vec3 city_pos, glm::vec3 planet_pos, glm::vec3 cam_pos);
    void CalculateCityPositions();

    void NewRender();
    void FocusCityView();

    glm::vec3 CalculateObjectPos(const entt::entity &);
    glm::vec3 CalculateCenteredObject(const entt::entity &);
    glm::vec3 CalculateCenteredObject(const glm::vec3 &);
    glm::vec3 TranslateToNormalized(const glm::vec3 &);

    void CalculateCamera();
    void MoveCamera(double deltaTime);

    void CheckResourceDistRender();

    glm::vec3 CalculateMouseRay(const glm::vec3 &ray_nds);
    float GetWindowRatio();

    void GenerateOrbitLines();

    // How much to scale the the star system.
    const double divider = 0.01;
    float window_ratio;

    glm::vec3 sun_position;
    glm::vec3 sun_color;

    engine::LayerRenderer renderer;

    int ship_icon_layer;
    int planet_icon_layer;
    int physical_layer;
    int skybox_layer;

    bool is_founding_city = false;
    bool is_rendering_founding_city = false;
    glm::vec3 city_founding_position;
    entt::entity on_planet;

    float view_scale = 10.f;
    // There are 2 scales, the AU scale and the 
};
}  // namespace systems
}  // namespace client
}  // namespace cqsp
