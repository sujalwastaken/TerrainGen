#include "TerrainRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * aPos);
    Normal = mat3(transpose(inverse(model))) * vec3(aNormal);
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 colorSnow;
uniform vec3 colorRock;
uniform vec3 colorGrass;
uniform vec3 colorDirt;
uniform vec3 colorSand;

uniform float heightSnow;
uniform float heightGrass;
uniform float heightDirt; 
uniform float slopeRock;

void main() {
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * vec3(1.0);
    
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0);
    
    float slope = 1.0 - norm.y; 
    float h = FragPos.y;
    
    // TEXTURE SPLATTING (Now with negative height support!)
    // Base is Sand. Transitions to Dirt, then Grass, then Snow.
    vec3 groundColor = mix(colorSand, colorDirt, smoothstep(heightDirt - 2.0, heightDirt + 2.0, h));
    groundColor = mix(groundColor, colorGrass, smoothstep(heightGrass - 5.0, heightGrass + 5.0, h));
    groundColor = mix(groundColor, colorSnow, smoothstep(heightSnow - 10.0, heightSnow + 10.0, h));
    
    float rockBlend = smoothstep(slopeRock - 0.1, slopeRock + 0.1, slope);
    vec3 finalMaterial = mix(groundColor, colorRock, rockBlend);
    
    vec3 result = (ambient + diffuse) * finalMaterial;
    FragColor = vec4(result, 1.0);
}
)";

TerrainRenderer::TerrainRenderer() : indexCount(0) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &EBO);
    SetupShaders();
}

TerrainRenderer::~TerrainRenderer() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
}

void TerrainRenderer::BindBuffers(unsigned int VBO, const std::vector<unsigned int>& indices) {
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    indexCount = indices.size();
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void TerrainRenderer::Draw(const glm::mat4& view, const glm::mat4& projection, const RenderSettings& settings) {
    glUseProgram(shaderProgram);

    // Set matrices
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Set texturing uniforms
    glUniform3fv(glGetUniformLocation(shaderProgram, "colorSnow"), 1, glm::value_ptr(settings.colorSnow));
    glUniform3fv(glGetUniformLocation(shaderProgram, "colorRock"), 1, glm::value_ptr(settings.colorRock));
    glUniform3fv(glGetUniformLocation(shaderProgram, "colorGrass"), 1, glm::value_ptr(settings.colorGrass));
    glUniform3fv(glGetUniformLocation(shaderProgram, "colorDirt"), 1, glm::value_ptr(settings.colorDirt));
    glUniform3fv(glGetUniformLocation(shaderProgram, "colorSand"), 1, glm::value_ptr(settings.colorSand));


    glUniform1f(glGetUniformLocation(shaderProgram, "heightSnow"), settings.heightSnow);
    glUniform1f(glGetUniformLocation(shaderProgram, "heightGrass"), settings.heightGrass);
    glUniform1f(glGetUniformLocation(shaderProgram, "slopeRock"), settings.slopeRock);
    glUniform1f(glGetUniformLocation(shaderProgram, "heightDirt"), settings.heightDirt);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

unsigned int TerrainRenderer::CompileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success; char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader Compilation Error:\n" << infoLog << std::endl;
    }
    return shader;
}

void TerrainRenderer::SetupShaders() {
    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}