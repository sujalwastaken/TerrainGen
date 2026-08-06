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

// --- GLOBALS (Changed from const so they can update on resize) ---
unsigned int windowWidth = 1280;
unsigned int windowHeight = 720;

// --- CAMERA STATE ---
glm::vec3 cameraPos = glm::vec3(0.0f, 100.0f, 150.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.5f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool uiMode = true;
bool firstMouse = true;
float yaw = -90.0f;
float pitch = -30.0f;
float lastX = windowWidth / 2.0f;
float lastY = windowHeight / 2.0f;

// --- TIMING ---
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// --- INPUT & RESIZE CALLBACKS ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Make sure the viewport matches the new window dimensions
    glViewport(0, 0, width, height);
    windowWidth = width;
    windowHeight = height;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        uiMode = !uiMode;
        if (uiMode) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true;
        }
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    if (uiMode) return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void processInput(GLFWwindow* window) {
    if (uiMode) return;

    float cameraSpeed = 75.0f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraUp;
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Realistic Terrain Generator", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set up Callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    TerrainGenerator generator;
    TerrainRenderer renderer;

    generator.LoadSeedMap("../../../assets/seed_map.png");
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

    RenderSettings renderSettings;

    lastGenerationTime = generator.Generate(
        baseMapInfluence, noiseFrequency, octaves, heightScale,
        applyErosion, erosionIterations, talusAngle, erosionRate
    );

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Terrain Dashboard");
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Press [ESC] to toggle Camera / UI Mode");
        ImGui::Separator();

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

        // Calculate dynamic aspect ratio to prevent stretching on resize
        float aspect = (float)windowWidth / (float)(windowHeight == 0 ? 1 : windowHeight); // Prevent divide-by-zero if minimized

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 1000.0f);

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