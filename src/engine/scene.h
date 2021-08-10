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

#include "engine/application.h"
#include "engine/engine.h"

namespace cqsp {
namespace engine {
class Scene {
 public:
    explicit Scene(Application& app);

    Application& GetApp() { return m_application; }

    virtual void Init() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Ui(float deltaTime) = 0;
    virtual void Render(float deltaTime) = 0;

 private:
    Application& m_application;
};

class EmptyScene : public Scene {
 public:
    explicit EmptyScene(Application& app);

    void Init();
    void Update(float deltaTime);
    void Ui(float deltaTime);
    void Render(float deltaTime);
};
}  // namespace engine
}  // namespace cqsp
