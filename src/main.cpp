#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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

    generator.LoadSeedMap("../assets/seed_map.png");
    renderer.BindBuffers(generator.GetVBO(), generator.GetIndices());

    float baseMapInfluence = 0.5f;
    float noiseFrequency = 0.02f;
    int octaves = 4;
    float heightScale = 30.0f;

    bool applyErosion = false;
    int erosionIterations = 10;
    float talusAngle = 0.5f;
    float erosionRate = 0.1f;

    bool wireframe = false;
    bool autoUpdate = true;
    float lastGenerationTime = 0.0f;

    // ADDED: Texturing Settings
    RenderSettings renderSettings;

    lastGenerationTime = generator.Generate(
        baseMapInfluence, noiseFrequency, octaves, heightScale,
        applyErosion, erosionIterations, talusAngle, erosionRate
    );

    // 5. Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Dashboard Window
        ImGui::Begin("Terrain Dashboard");
        bool needsUpdate = false;

        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Generation Time: %.3f ms", lastGenerationTime);
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Procedural Generation", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::SliderFloat("Map Influence", &baseMapInfluence, 0.0f, 1.0f)) needsUpdate = true;
            if (ImGui::SliderFloat("Noise Freq", &noiseFrequency, 0.001f, 0.1f)) needsUpdate = true;
            if (ImGui::SliderInt("Noise Octaves", &octaves, 1, 8)) needsUpdate = true;
            if (ImGui::SliderFloat("Height Scale", &heightScale, 5.0f, 100.0f)) needsUpdate = true;
        }

        if (ImGui::CollapsingHeader("Thermal Erosion")) {
            if (ImGui::Checkbox("Apply Erosion", &applyErosion)) needsUpdate = true;
            if (applyErosion) {
                if (ImGui::SliderInt("Iterations", &erosionIterations, 1, 100)) needsUpdate = true;
                if (ImGui::SliderFloat("Talus Angle", &talusAngle, 0.1f, 2.0f)) needsUpdate = true;
                if (ImGui::SliderFloat("Erosion Rate", &erosionRate, 0.01f, 0.5f)) needsUpdate = true;
            }
        }

        // ADDED: Texturing & Biome Controls
        if (ImGui::CollapsingHeader("Biome & Splatting", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Color Palette");
            ImGui::ColorEdit3("Snow Color", glm::value_ptr(renderSettings.colorSnow));
            ImGui::ColorEdit3("Rock Color", glm::value_ptr(renderSettings.colorRock));
            ImGui::ColorEdit3("Grass Color", glm::value_ptr(renderSettings.colorGrass));
            ImGui::ColorEdit3("Dirt Color", glm::value_ptr(renderSettings.colorDirt));

            ImGui::Text("Thresholds");
            ImGui::SliderFloat("Snow Height", &renderSettings.heightSnow, 0.0f, 100.0f);
            ImGui::SliderFloat("Grass Height", &renderSettings.heightGrass, 0.0f, 40.0f);
            ImGui::SliderFloat("Rock Steepness", &renderSettings.slopeRock, 0.0f, 1.0f);
        }

        ImGui::Separator();
        ImGui::Checkbox("Auto-Update on Drag", &autoUpdate);
        if (ImGui::Checkbox("Wireframe Mode", &wireframe)) {
            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        }

        if ((needsUpdate && autoUpdate) || ImGui::Button("Force Regenerate")) {
            lastGenerationTime = generator.Generate(
                baseMapInfluence, noiseFrequency, octaves, heightScale,
                applyErosion, erosionIterations, talusAngle, erosionRate
            );
        }
        ImGui::End();

        glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Simple rotating camera (Will be replaced in Step 2!)
        float time = (float)glfwGetTime();
        float camX = sin(time * 0.2f) * 150.0f;
        float camZ = cos(time * 0.2f) * 150.0f;
        glm::mat4 view = glm::lookAt(glm::vec3(camX, 100.0f, camZ), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);

        // Render with new settings
        renderer.Draw(view, projection, renderSettings);

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