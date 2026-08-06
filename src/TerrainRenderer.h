#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "TerrainGenerator.h"

class TerrainRenderer {
public:
    TerrainRenderer();
    ~TerrainRenderer();

    void UpdateMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    void Draw(const glm::mat4& view, const glm::mat4& projection);

private:
    unsigned int VAO, VBO, EBO, shaderProgram;
    unsigned int indexCount;

    void SetupShaders();
    unsigned int CompileShader(unsigned int type, const char* source);
};