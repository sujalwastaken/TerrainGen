#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "TerrainGenerator.h"

class TerrainRenderer {
public:
    TerrainRenderer();
    ~TerrainRenderer();

    void BindBuffers(unsigned int VBO, const std::vector<unsigned int>& indices);
    void Draw(const glm::mat4& view, const glm::mat4& projection);

private:
    unsigned int VAO, EBO, shaderProgram;
    unsigned int indexCount;

    void SetupShaders();
    unsigned int CompileShader(unsigned int type, const char* source);
};