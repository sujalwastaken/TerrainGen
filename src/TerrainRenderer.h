#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "TerrainGenerator.h"

// Define our texturing parameters
struct RenderSettings {
    glm::vec3 colorSnow = glm::vec3(0.9f, 0.9f, 0.9f);
    glm::vec3 colorRock = glm::vec3(0.35f, 0.35f, 0.35f);
    glm::vec3 colorGrass = glm::vec3(0.2f, 0.45f, 0.15f);
    glm::vec3 colorDirt = glm::vec3(0.35f, 0.28f, 0.2f);

    float heightSnow = 40.0f;
    float heightGrass = 10.0f;
    float slopeRock = 0.2f; // 0.0 is flat, 1.0 is vertical wall
};

class TerrainRenderer {
public:
    TerrainRenderer();
    ~TerrainRenderer();

    void BindBuffers(unsigned int VBO, const std::vector<unsigned int>& indices);

    // Updated Draw function to accept render settings
    void Draw(const glm::mat4& view, const glm::mat4& projection, const RenderSettings& settings);

private:
    unsigned int VAO, EBO, shaderProgram;
    unsigned int indexCount;

    void SetupShaders();
    unsigned int CompileShader(unsigned int type, const char* source);
};