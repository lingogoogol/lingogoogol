#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include "tile.h"
#include "value.h"
#include "light.h"

Tile::Tile() :dir_lights{}, point_lights{}, spotlights{} {
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    float buffer_data[]{
        -1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(buffer_data), buffer_data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
	return;
}

void Tile::render(World_data* data) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_ONE, GL_ONE);
    glUseProgram(data->shader.tile_ambient);
    glBindVertexArray(VAO);
    glUniform1i(glGetUniformLocation(data->shader.tile_ambient, "color"), 0);
    glUniform3fv(glGetUniformLocation(data->shader.tile_ambient, "ambient_color"), 1, glm::value_ptr(data->ambient_color));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, data->color);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    for (int i{ 0 }; i < dir_lights.size(); i++) {
        ;
    }
    for (int i{ 0 }; i < point_lights.size(); i++) {
        glUseProgram(data->shader.tile_point);
        glBindVertexArray(VAO);
        glUniform1i(glGetUniformLocation(data->shader.tile_point, "pos_shininess"), 0);
        glUniform1i(glGetUniformLocation(data->shader.tile_point, "normal_specular_strength"), 1);
        glUniform1i(glGetUniformLocation(data->shader.tile_point, "color"), 2);
        glUniform3fv(glGetUniformLocation(data->shader.tile_point, "light.pos"), 1, glm::value_ptr(point_lights[i]->pos));
        glUniform3fv(glGetUniformLocation(data->shader.tile_point, "light.color"), 1, glm::value_ptr(point_lights[i]->color));
        glUniform3fv(glGetUniformLocation(data->shader.tile_point, "camera_pos"), 1, glm::value_ptr(data->camera_pos));
        glUniform3fv(glGetUniformLocation(data->shader.tile_point, "camera_dir"), 1, glm::value_ptr(data->camera_dir));
        glActiveTexture(GL_TEXTURE0);
        glActiveTexture(GL_TEXTURE1);
        glActiveTexture(GL_TEXTURE2);
    }
    for (int i{ 0 }; i < spotlights.size(); i++) {
        ;
    }
	return;
}