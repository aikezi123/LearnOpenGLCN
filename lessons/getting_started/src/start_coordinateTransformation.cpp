#include "start_coordinateTransformation.h"
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <stb_image.h>
#include <iostream>
#include <shader/Shader.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


int transform() {

    // ———————————— 1. 初始化glfw和glad库 ————————————
    // 初始化glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow * window = glfwCreateWindow(800, 600, "LearnOpenGLCN", nullptr, nullptr);

    if (window == nullptr) {
        std::cout << "faile to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height) {
        glViewport(0, 0, width, height);
    });

    // 初始化glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // 创建顶点数组
    float vertics[] = {
        0.5f, 0.5f, 0.0f,   1.0f, 1.0f,         // 右上角
        0.5f, -0.5f, 0.0f,  1.0f, 0.0f,         // 右下角
        -0.5f, 0.5f, 0.0f,  0.0f, 1.0f,         // 左上角
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f          // 左下角
    };

    // 创建索引数组
    unsigned int indices[] = {
        0, 1, 3,    // 第一个三角形
        1, 2, 3     // 第二个三角形
    };

    // 创建着色器
    const char *vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;
        layout (location = 2) in vec2 aTexCoord;

        out vec3 ourColor;
        out vec2 TexCoord;

        uniform mat4 transform;

        void main()
        {
            gl_Position = transform * vec4(aPos, 1.0f);
            TexCoord = vec2(aTexCoord.x, aTexCoord.y);
            ourColor = aColor;
        }
    )";

    const char *fragShaderSource = R"(
        #version 330

    )";



    // 创建VAO、VBO、EBO
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);

    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertics), vertics, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // glVertexAttribPointer();



    return 0;
}
