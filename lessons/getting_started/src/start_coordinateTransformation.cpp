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
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
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
        layout (location = 1) in vec2 aTexCoord;

        out vec2 TexCoord;

        uniform mat4 transform;

        void main()
        {
            gl_Position = transform * vec4(aPos, 1.0f);
            TexCoord = vec2(aTexCoord.x, aTexCoord.y);
        }
    )";

    const char *fragShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;

        uniform sampler2D uTexture;

        void main()
        {
            FragColor = texture(uTexture, TexCoord);
        }
    )";

    unsigned int vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderID, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShaderID);

    unsigned int fragShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShaderID, 1, &fragShaderSource, nullptr);
    glCompileShader(fragShaderID);

    unsigned int programID = glCreateProgram();
    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragShaderID);
    glLinkProgram(programID);


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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 创建纹理对象
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //加载并生成纹理
    int width, height, nrChannels;
    std::string texPath = LEARNOPENGL_ASSET_DIR + std::string("/textures/getting_started/texture_texture/container.jpg");
    unsigned char *data = stbi_load(texPath.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    }
    stbi_image_free(data);


    // 开始绘制
    glUseProgram(programID);
    // 获取uTexture在glsl着色器代码中对应的location
    unsigned int location = glGetUniformLocation(programID, "uTexture");
    // 把location对应的对象也就是把uTexture的值设为0。
    // uTexture = 0的含义：由于uTexture的类型是unifrom sampler 2D，因此它的值不是颜色、矩阵或坐标，而是Texture Unit编号。
    // 因此，glUniform1i(loc, 0)表示uTexture = 0，也就是这个sample2D从Texture Unit 0采样。
    // 真正的纹理绑定还得靠glActiveTexture(GL_TEXTURE0)；glBindTexture(GL_TEXTURE_2D, textureID)。
    glUniform1i(location, 0);
    
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    
        // ———————————————— 矩阵变换，实际顺序为先缩放、后旋转、再平移 ———————————————
        // 构造4*4的单位变换矩阵
        glm::mat4 trans = glm::mat4(1.0f);
        // 变换矩阵左乘平移矩阵
        trans = glm::translate(trans, glm::vec3(0.3f, 0.2f, 0.0f));
        // 变换矩阵左乘旋转矩阵
        trans = glm::rotate(trans, static_cast<float>(glfwGetTime()), glm::vec3(0.0f, 0.0f, 1.0f));
        // 变换矩阵左乘缩放矩阵
        trans = glm::scale(trans, glm::vec3(0.8f, 0.8f, 1.0f));
    
        glUseProgram(programID);

        // 查询programID这个program里名字叫“transform”的unifrom变量，它的location是多少
        unsigned int transformLocation = glGetUniformLocation(programID, "transform");
        // 把变换矩阵传给当前program的unifrom mat4 transform中
        glUniformMatrix4fv(
            transformLocation,         // unifrom location
            1,                         // 上传1个mat4
            GL_FALSE,                  // 不转置矩阵
            glm::value_ptr(trans)      // 指向矩阵首元素的float指针
        );    
        
        // 将当前的活动纹理单元切换为Texture Unit 0
        glActiveTexture(GL_TEXTURE0);
        // 将texture这个Texture Object绑定到当前活动的Texture Unit的GL_TEXTURE_2D绑定点上
        glBindTexture(GL_TEXTURE_2D, texture);
    
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    return 0;
}
