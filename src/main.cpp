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
    // 1. Initialize GLFW & GLAD (Omitted exact boilerplate for brevity, same as previous step)
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Terrain Generator", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST); // Crucial for 3D

    // 2. Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 3. Setup Terrain Classes
    TerrainGenerator generator;
    TerrainRenderer renderer;

    // Default Parameters
    float baseMapInfluence = 0.5f;
    float noiseFrequency = 0.02f;
    int octaves = 4;
    float heightScale = 30.0f;
    bool wireframe = false;
    std::string mapPath = "../../../assets/seed_map.png";

    // Initial Generation
    generator.Generate(mapPath, baseMapInfluence, noiseFrequency, octaves, heightScale);
    renderer.UpdateMesh(generator.GetVertices(), generator.GetIndices());

    // 4. Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Dashboard
        ImGui::Begin("Terrain Settings");
        bool needsUpdate = false;

        if (ImGui::SliderFloat("Base Map Influence", &baseMapInfluence, 0.0f, 1.0f)) needsUpdate = true;
        if (ImGui::SliderFloat("Noise Frequency", &noiseFrequency, 0.001f, 0.1f)) needsUpdate = true;
        if (ImGui::SliderInt("Noise Octaves", &octaves, 1, 8)) needsUpdate = true;
        if (ImGui::SliderFloat("Height Scale", &heightScale, 5.0f, 100.0f)) needsUpdate = true;

        ImGui::Separator();
        if (ImGui::Checkbox("Wireframe Mode", &wireframe)) {
            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        }

        if (needsUpdate || ImGui::Button("Force Regenerate")) {
            generator.Generate(mapPath, baseMapInfluence, noiseFrequency, octaves, heightScale);
            renderer.UpdateMesh(generator.GetVertices(), generator.GetIndices());
        }
        ImGui::End();

        // Render OpenGL
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Simple rotating camera to view the terrain
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