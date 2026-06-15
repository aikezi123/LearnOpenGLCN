#include "start_textrues.h"
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <stb_image.h>
#include <iostream>
#include "Shader.hpp"

int start_textures() {

    // ———————————— 1. 初始化glfw和glad库 ————————————
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow * window = glfwCreateWindow(1920, 1080, "LearnOpenGLCN", nullptr, nullptr);

    if (window == nullptr) {
        std::cout << "faile to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height) {
        glViewport(0, 0, width, height);
    });

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // ————————————— 2. 准备顶点数组 ——————————————
    float vertices[] = {
    //     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -
         0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // 右上
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // 右下
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // 左下
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // 左上
    };

    unsigned int indices[] = {
        0, 1, 3,   // 第一个三角形
        1, 2, 3    // 第二个三角形
    };


    // ———————————— 3. 绑定VAO、VBO、EBO ————————————
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 6));
    glEnableVertexAttribArray(2);


    // ———————————— 4. 绑定纹理 ————————————
    // 1. 生成纹理
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // 2. 设置当前绑定的纹理对象设置环绕、过滤方式
    // s轴(横坐标)的纹理环绕方式。设置为重复纹理图像。
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // t轴(纵坐标)的纹理环绕方式。设置为重复纹理图像
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // 决定缩小时的采样方式。设置为三线性过滤。
    // 纹理缩小时：
    // 1. 根据纹理在屏幕上的大小选择合适的 Mipmap 层级；
    // 2. 在每个层级内部做线性过滤；
    // 3. 在两个相邻 Mipmap 层级之间也做线性混合。
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // 决定放大时的采样方式。设置为三线性过滤。
    // 纹理放大时：
    // 1. 根据纹理在屏幕上的大小选择合适的 Mipmap 层级；
    // 2. 在每个层级内部做线性过滤；
    // 3. 在两个相邻 Mipmap 层级之间也做线性混合。
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    // 3. 加载纹理图片
    int width, height, nrChannels;
    std::string texPath = LEARNOPENGL_ASSET_DIR + std::string("/textures/getting_started/texture_texture/container.jpg");
    unsigned char *data = stbi_load(texPath.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        // 再GPU中为纹理分配存储空间，并把data指向的CPU像素数据复制到GPU
        // 参数1 : GL_TEXTURE_2D,表示正在定义一个二维纹理, 它会作用于当前通过以下代码绑定的对象
        // 参数2 : 0。表示要上传的是 Mipmap 的第 0 层，也就是原始最高分辨率纹理。
        // 参数3 : GL_RGB。这是内部格式，表示 GPU 应该怎样存储这张纹理。GPU按照红、绿、蓝三个颜色通道保存纹理。
        // 参数4、5 : width 和 height。表示纹理的宽度和高度。它们由 stbi_load() 填写。
        // 参数6 : 0。表示纹理边框宽度。
        // 参数7 : GL_RGB。 这是源数据格式，描述 data 中的像素是怎样排列的。
        // 参数8 : GL_UNSIGNED_BYTE。表示 data 中每个颜色分量的数据类型。
        // 参数9 : data。是图片解码后的实际像素数组。
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        // 生成多级渐远纹理，准备数据,但是生成了不代表一定会使用。例如：
        // Level 0：1024 × 1024
        // Level 1： 512 × 512
        // Level 2： 256 × 256
        // ....
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture" << std::endl;
    }

    // 释放图像内存
    stbi_image_free(data);

    
    // ———————————— 5. 绘制图像 ———————————————
    std::string shaderPath = LEARNOPENGL_ASSET_DIR + std::string("/shaders/getting_started/shader_texture");
    std::string vertexShaderPath = shaderPath + "/texture.vert";
    std::string fragShaderPath = shaderPath + "/texture.frag";
    learnopengl::infrastructure::Shader myShader(vertexShaderPath.c_str(), fragShaderPath.c_str());
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindTexture(GL_TEXTURE_2D, textureID);

        myShader.use();

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();

    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glfwTerminate();


    return 0;
}