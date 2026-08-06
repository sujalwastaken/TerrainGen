#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
};

class TerrainGenerator {
public:
    void Generate(const std::string& imagePath, float baseInfluence, float noiseFreq, int octaves, float heightScale);

    const std::vector<Vertex>& GetVertices() const { return vertices; }
    const std::vector<unsigned int>& GetIndices() const { return indices; }

private:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    void CalculateNormals(int width, int depth);
};