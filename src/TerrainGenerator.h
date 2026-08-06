#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec4 Position; // vec4 for 16-byte GPU alignment
    glm::vec4 Normal;   // vec4 for 16-byte GPU alignment
};

class TerrainGenerator {
public:
    TerrainGenerator();
    ~TerrainGenerator();

    // Now returns the time taken in milliseconds
    float Generate(float baseInfluence, float noiseFreq, int octaves, float heightScale);
    void LoadSeedMap(const std::string& imagePath);
    void GenerateTopology(int width, int depth);

    unsigned int GetVBO() const { return VBO; }
    int GetIndexCount() const { return indices.size(); }
    const std::vector<unsigned int>& GetIndices() const { return indices; }

private:
    unsigned int VBO;
    unsigned int computeShader;
    unsigned int seedTexture;

    int gridWidth = 256;
    int gridDepth = 256;

    std::vector<unsigned int> indices;

    unsigned int CompileComputeShader();
};