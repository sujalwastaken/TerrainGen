#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "TerrainGenerator.h"
#include "TerrainRenderer.h"

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

int main() {
    // 1. Initialize GLFW & GLAD
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Realistic Terrain Generator", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);

    // 2. Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    // 3. Setup Architecture
    TerrainGenerator generator;
    TerrainRenderer renderer;

    generator.LoadSeedMap("../../../assets/seed_map.png");
    renderer.BindBuffers(generator.GetVBO(), generator.GetIndices());

    // Dashboard State
    float baseMapInfluence = 0.5f;
    float noiseFrequency = 0.02f;
    int octaves = 4;
    float heightScale = 30.0f;
    bool wireframe = false;
    bool autoUpdate = true;
    float lastGenerationTime = 0.0f;

    // Run initial generation pass
    lastGenerationTime = generator.Generate(baseMapInfluence, noiseFrequency, octaves, heightScale);

    // 4. Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Dashboard
        ImGui::Begin("Terrain Settings");
        bool needsUpdate = false;

        ImGui::Text("GPU Compute Performance");
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Generation Time: %.3f ms", lastGenerationTime);
        ImGui::Separator();

        if (ImGui::SliderFloat("Base Map Influence", &baseMapInfluence, 0.0f, 1.0f)) needsUpdate = true;
        if (ImGui::SliderFloat("Noise Frequency", &noiseFrequency, 0.001f, 0.1f)) needsUpdate = true;
        if (ImGui::SliderInt("Noise Octaves", &octaves, 1, 8)) needsUpdate = true;
        if (ImGui::SliderFloat("Height Scale", &heightScale, 5.0f, 100.0f)) needsUpdate = true;

        ImGui::Separator();
        ImGui::Checkbox("Auto-Update on Drag", &autoUpdate);
        if (ImGui::Checkbox("Wireframe Mode", &wireframe)) {
            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        }

        if ((needsUpdate && autoUpdate) || ImGui::Button("Force Regenerate")) {
            lastGenerationTime = generator.Generate(baseMapInfluence, noiseFrequency, octaves, heightScale);
        }
        ImGui::End();

        // Render OpenGL
        glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Simple rotating camera
        float time = (float)glfwGetTime();
        float camX = sin(time * 0.2f) * 150.0f;
        float camZ = cos(time * 0.2f) * 150.0f;
        glm::mat4 view = glm::lookAt(glm::vec3(camX, 100.0f, camZ), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);

        renderer.Draw(view, projection);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}