#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "TerrainGenerator.h"
#include "FastNoiseLite.h"
#include <iostream>

void TerrainGenerator::Generate(const std::string& imagePath, float baseInfluence, float noiseFreq, int octaves, float heightScale) {
    vertices.clear();
    indices.clear();

    int width, depth, channels;
    // Force grayscale (1 channel) for elevation data
    unsigned char* data = stbi_load(imagePath.c_str(), &width, &depth, &channels, 1);

    // Fallback if image missing
    if (!data) {
        std::cerr << "Failed to load elevation map, using default grid size (128x128).\n";
        width = 128;
        depth = 128;
    }

    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(octaves);
    noise.SetFrequency(noiseFreq);

    // Center the terrain at (0,0,0)
    float halfWidth = width / 2.0f;
    float halfDepth = depth / 2.0f;

    // 1. Generate Vertices
    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x < width; ++x) {
            float baseHeight = 0.0f;
            if (data) {
                // Read pixel value (0-255) and normalize to 0.0-1.0
                unsigned char pixelValue = data[z * width + x];
                baseHeight = (pixelValue / 255.0f) * 2.0f - 1.0f; // -1 to 1
            }

            // Generate noise (-1 to 1)
            float noiseHeight = noise.GetNoise((float)x, (float)z);

            // Blend them based on UI slider
            float finalHeight = (baseHeight * baseInfluence) + (noiseHeight * (1.0f - baseInfluence));
            finalHeight *= heightScale;

            Vertex v;
            v.Position = glm::vec3(x - halfWidth, finalHeight, z - halfDepth);
            v.Normal = glm::vec3(0.0f, 1.0f, 0.0f); // Default, calculated later
            vertices.push_back(v);
        }
    }

    if (data) stbi_image_free(data);

    // 2. Generate Indices (Triangles)
    for (int z = 0; z < depth - 1; ++z) {
        for (int x = 0; x < width - 1; ++x) {
            int topLeft = (z * width) + x;
            int topRight = topLeft + 1;
            int bottomLeft = ((z + 1) * width) + x;
            int bottomRight = bottomLeft + 1;

            // Triangle 1
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            // Triangle 2
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    // 3. Calculate Normals for lighting
    CalculateNormals(width, depth);
}

void TerrainGenerator::CalculateNormals(int width, int depth) {
    // Basic central difference for grid normals
    for (int z = 1; z < depth - 1; ++z) {
        for (int x = 1; x < width - 1; ++x) {
            float hL = vertices[z * width + (x - 1)].Position.y;
            float hR = vertices[z * width + (x + 1)].Position.y;
            float hD = vertices[(z - 1) * width + x].Position.y;
            float hU = vertices[(z + 1) * width + x].Position.y;

            glm::vec3 normal;
            normal.x = hL - hR;
            normal.y = 2.0f; // Grid spacing multiplier
            normal.z = hD - hU;

            vertices[z * width + x].Normal = glm::normalize(normal);
        }
    }
}