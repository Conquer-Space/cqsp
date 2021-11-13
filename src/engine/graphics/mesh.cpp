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
#include "engine/graphics/mesh.h"

#include <glad/glad.h>

cqsp::engine::Mesh::Mesh() {
    mode = GL_TRIANGLES;
}

cqsp::engine::Mesh::~Mesh() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void cqsp::engine::Mesh::Draw() {
    glBindVertexArray(VAO);
    switch (buffer_type) {
        case DrawType::ELEMENTS:
            glDrawElements(mode, indicies, GL_UNSIGNED_INT, 0);
            break;
        case DrawType::ARRAYS:
            glDrawArrays(mode, 0, indicies);
    }
    glBindVertexArray(0);
}
