#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "TerrainGenerator.h"
#include <glad/glad.h>
#include <iostream>
#include <chrono>

// --- SHADER 1: BASE GENERATION ---
const char* computeShaderSource = R"(
#version 430 core
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

struct Vertex {
    vec4 Position;
    vec4 Normal;
};

layout(std430, binding = 0) buffer TerrainBuffer {
    Vertex vertices[];
};

uniform sampler2D seedMap;
uniform int width;
uniform int depth;
uniform float baseInfluence;
uniform float noiseFreq;
uniform int octaves;
uniform float heightScale;

vec2 hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(dot(hash(i + vec2(0.0,0.0)), f - vec2(0.0,0.0)),
                   dot(hash(i + vec2(1.0,0.0)), f - vec2(1.0,0.0)), u.x),
               mix(dot(hash(i + vec2(0.0,1.0)), f - vec2(0.0,1.0)),
                   dot(hash(i + vec2(1.0,1.0)), f - vec2(1.0,1.0)), u.x), u.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    vec2 shift = vec2(100.0);
    for (int i = 0; i < octaves; ++i) {
        value += amplitude * noise(p);
        p = p * 2.0 + shift;
        amplitude *= 0.5;
    }
    return value;
}

float GetHeight(vec2 pos) {
    vec2 uv = vec2(pos.x / float(width - 1), pos.y / float(depth - 1));
    float baseHeight = texture(seedMap, uv).r * 2.0 - 1.0;
    float nHeight = fbm(pos * noiseFreq);
    return ((baseHeight * baseInfluence) + (nHeight * (1.0 - baseInfluence))) * heightScale;
}

void main() {
    uint x = gl_GlobalInvocationID.x;
    uint z = gl_GlobalInvocationID.y;
    
    if (x >= uint(width) || z >= uint(depth)) return;
    
    uint index = z * uint(width) + x;
    
    float halfW = width / 2.0;
    float halfD = depth / 2.0;
    
    vec2 pos = vec2(float(x), float(z));
    float h = GetHeight(pos);
    
    vertices[index].Position = vec4(float(x) - halfW, h, float(z) - halfD, 1.0);
    
    // Clamp to prevent wrapping walls at edges
    vec2 pL = clamp(pos + vec2(-1.0, 0.0), vec2(0.0), vec2(width - 1.0, depth - 1.0));
    vec2 pR = clamp(pos + vec2(1.0, 0.0), vec2(0.0), vec2(width - 1.0, depth - 1.0));
    vec2 pD = clamp(pos + vec2(0.0, -1.0), vec2(0.0), vec2(width - 1.0, depth - 1.0));
    vec2 pU = clamp(pos + vec2(0.0, 1.0), vec2(0.0), vec2(width - 1.0, depth - 1.0));
    
    float hL = GetHeight(pL);
    float hR = GetHeight(pR);
    float hD = GetHeight(pD);
    float hU = GetHeight(pU);
    
    vec3 normal = normalize(vec3(hL - hR, 2.0, hD - hU));
    vertices[index].Normal = vec4(normal, 0.0);
}
)";

// --- SHADER 2: THERMAL EROSION (FIXED) ---
const char* thermalErosionShaderSource = R"(
#version 430 core
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

struct Vertex {
    vec4 Position;
    vec4 Normal;
};

layout(std430, binding = 0) buffer TerrainBuffer {
    Vertex vertices[];
};

uniform int width;
uniform int depth;
uniform float talusAngle;
uniform float erosionRate;

void main() {
    uint x = gl_GlobalInvocationID.x;
    uint z = gl_GlobalInvocationID.y;
    
    if (x >= uint(width) || z >= uint(depth)) return;
    
    uint index = z * uint(width) + x;
    float h = vertices[index].Position.y;
    
    // Bulletproof integer clamping to fix edge stretch
    uint xL = max(x, 1u) - 1u;
    uint xR = min(x + 1u, uint(width - 1));
    uint zD = max(z, 1u) - 1u;
    uint zU = min(z + 1u, uint(depth - 1));
    
    float hL = vertices[z * uint(width) + xL].Position.y;
    float hR = vertices[z * uint(width) + xR].Position.y;
    float hD = vertices[zD * uint(width) + x].Position.y;
    float hU = vertices[zU * uint(width) + x].Position.y;
    
    float heightChange = 0.0;
    
    // Thread-safe gather: if neighbor is higher, we gain (+). If lower, we lose (-).
    float diffL = hL - h;
    float diffR = hR - h;
    float diffD = hD - h;
    float diffU = hU - h;
    
    // Move material only if it exceeds the maximum stable steepness (talus angle)
    if (abs(diffL) > talusAngle) heightChange += (diffL - sign(diffL) * talusAngle) * erosionRate;
    if (abs(diffR) > talusAngle) heightChange += (diffR - sign(diffR) * talusAngle) * erosionRate;
    if (abs(diffD) > talusAngle) heightChange += (diffD - sign(diffD) * talusAngle) * erosionRate;
    if (abs(diffU) > talusAngle) heightChange += (diffU - sign(diffU) * talusAngle) * erosionRate;
    
    // Apply average change (divided by 4 to prevent grid-checkerboarding spikes)
    vertices[index].Position.y += heightChange * 0.25; 
}
)";

// --- SHADER 3: RECALCULATE NORMALS (FIXED) ---
const char* normalComputeShaderSource = R"(
#version 430 core
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

struct Vertex {
    vec4 Position;
    vec4 Normal;
};

layout(std430, binding = 0) buffer TerrainBuffer {
    Vertex vertices[];
};

uniform int width;
uniform int depth;

void main() {
    uint x = gl_GlobalInvocationID.x;
    uint z = gl_GlobalInvocationID.y;
    
    if (x >= uint(width) || z >= uint(depth)) return;
    
    uint index = z * uint(width) + x;
    
    // Use the exact same integer clamp as erosion for perfect edges
    uint xL = max(x, 1u) - 1u;
    uint xR = min(x + 1u, uint(width - 1));
    uint zD = max(z, 1u) - 1u;
    uint zU = min(z + 1u, uint(depth - 1));
    
    float hL = vertices[z * uint(width) + xL].Position.y;
    float hR = vertices[z * uint(width) + xR].Position.y;
    float hD = vertices[zD * uint(width) + x].Position.y;
    float hU = vertices[zU * uint(width) + x].Position.y;
    
    vec3 normal = normalize(vec3(hL - hR, 2.0, hD - hU));
    vertices[index].Normal = vec4(normal, 0.0);
}
)";

TerrainGenerator::TerrainGenerator() {
    computeShader = CompileComputeShader(computeShaderSource);
    erosionShader = CompileComputeShader(thermalErosionShaderSource);
    normalShader = CompileComputeShader(normalComputeShaderSource);

    glGenBuffers(1, &VBO);
    glGenTextures(1, &seedTexture);
}

TerrainGenerator::~TerrainGenerator() {
    glDeleteBuffers(1, &VBO);
    glDeleteTextures(1, &seedTexture);

    if (computeShader) glDeleteProgram(computeShader);
    if (erosionShader) glDeleteProgram(erosionShader);
    if (normalShader)  glDeleteProgram(normalShader);
}

void TerrainGenerator::LoadSeedMap(const std::string& imagePath) {
    int w, h, channels;
    unsigned char* data = stbi_load(imagePath.c_str(), &w, &h, &channels, 1);

    if (data) {
        gridWidth = w;
        gridDepth = h;

        glBindTexture(GL_TEXTURE_2D, seedTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else {
        std::cerr << "Failed to load image: " << imagePath << std::endl;
    }

    size_t vertexCount = gridWidth * gridDepth;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, VBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, vertexCount * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, VBO);

    GenerateTopology(gridWidth, gridDepth);
}

float TerrainGenerator::Generate(float baseInfluence, float noiseFreq, int octaves, float heightScale,
    bool applyErosion, int erosionIterations,
    float talusAngle, float erosionRate) {

    if (!computeShader || !erosionShader || !normalShader) return 0.0f;

    auto startTime = std::chrono::high_resolution_clock::now();
    int workgroupsX = (gridWidth + 15) / 16;
    int workgroupsY = (gridDepth + 15) / 16;

    // PASS 1: Base Generation
    glUseProgram(computeShader);
    glUniform1i(glGetUniformLocation(computeShader, "seedMap"), 0);
    glUniform1i(glGetUniformLocation(computeShader, "width"), gridWidth);
    glUniform1i(glGetUniformLocation(computeShader, "depth"), gridDepth);
    glUniform1f(glGetUniformLocation(computeShader, "baseInfluence"), baseInfluence);
    glUniform1f(glGetUniformLocation(computeShader, "noiseFreq"), noiseFreq);
    glUniform1i(glGetUniformLocation(computeShader, "octaves"), octaves);
    glUniform1f(glGetUniformLocation(computeShader, "heightScale"), heightScale);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, seedTexture);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, VBO);

    glDispatchCompute(workgroupsX, workgroupsY, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // PASS 2 & 3: Erosion and Normal Recalculation
    if (applyErosion) {
        glUseProgram(erosionShader);
        glUniform1i(glGetUniformLocation(erosionShader, "width"), gridWidth);
        glUniform1i(glGetUniformLocation(erosionShader, "depth"), gridDepth);
        glUniform1f(glGetUniformLocation(erosionShader, "talusAngle"), talusAngle);
        glUniform1f(glGetUniformLocation(erosionShader, "erosionRate"), erosionRate);

        for (int i = 0; i < erosionIterations; ++i) {
            glDispatchCompute(workgroupsX, workgroupsY, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        glUseProgram(normalShader);
        glUniform1i(glGetUniformLocation(normalShader, "width"), gridWidth);
        glUniform1i(glGetUniformLocation(normalShader, "depth"), gridDepth);
        glDispatchCompute(workgroupsX, workgroupsY, 1);
    }

    glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    glFinish();

    auto endTime = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float, std::milli>(endTime - startTime).count();
}

void TerrainGenerator::GenerateTopology(int width, int depth) {
    indices.clear();
    for (int z = 0; z < depth - 1; ++z) {
        for (int x = 0; x < width - 1; ++x) {
            int topLeft = (z * width) + x;
            int topRight = topLeft + 1;
            int bottomLeft = ((z + 1) * width) + x;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft); indices.push_back(bottomLeft); indices.push_back(topRight);
            indices.push_back(topRight); indices.push_back(bottomLeft); indices.push_back(bottomRight);
        }
    }
}

unsigned int TerrainGenerator::CompileComputeShader(const char* source) {
    unsigned int shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[1024];

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        std::cerr << "--- Compute Shader Compilation Failed ---\n" << infoLog << std::endl;
        return 0;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, NULL, infoLog);
        std::cerr << "--- Shader Program Linking Failed ---\n" << infoLog << std::endl;
        return 0;
    }

    glDeleteShader(shader);
    return program;
}