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

    // Updated signature to include erosion parameters
    float Generate(float baseInfluence, float noiseFreq, int octaves, float heightScale,
        bool applyErosion = false, int erosionIterations = 10,
        float talusAngle = 0.5f, float erosionRate = 0.1f);

    void LoadSeedMap(const std::string& imagePath);
    void GenerateTopology(int width, int depth);

    unsigned int GetVBO() const { return VBO; }
    int GetIndexCount() const { return indices.size(); }
    const std::vector<unsigned int>& GetIndices() const { return indices; }
    std::vector<float> GetSlopeDistribution(int numBins);

private:
    unsigned int VBO;
    unsigned int computeShader;
    unsigned int erosionShader; // New shader for physics
    unsigned int normalShader;
    unsigned int seedTexture;

    int gridWidth = 256;
    int gridDepth = 256;

    std::vector<unsigned int> indices;

    // Updated signature to accept the shader string
    unsigned int CompileComputeShader(const char* source);
};