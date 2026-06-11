#include <shader_exercise.h>
#include <learnopengl/Shader.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

namespace
{
    constexpr unsigned int SCREEN_WIDTH = 800;
    constexpr unsigned int SCREEN_HEIGHT = 600;

    enum class ShaderExercise
    {
        FlipTriangle = 1,
        MoveTriangle = 2,
        PositionAsColor = 3
    };

    // 在这里切换练习：
    // FlipTriangle    : 三角形上下翻转
    // MoveTriangle    : 使用 uniform float xOffset 移动三角形
    // PositionAsColor : 使用顶点位置作为颜色
    constexpr ShaderExercise CURRENT_EXERCISE = ShaderExercise::MoveTriangle;

    void framebufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    void processInput(GLFWwindow* window)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }
    }

    std::string getShaderPath(const std::string& fileName)
    {
        return std::string(LEARNOPENGL_ASSET_DIR) +
               "/shaders/getting_started/shader_exercise/" +
               fileName;
    }
}

namespace learnopengl::getting_started
{
    void runShaderExercise()
    {
        // —————————————— 1. 创建窗口 ————————————————
        if (!glfwInit())
        {
            std::cout << "Failed to initialize GLFW" << std::endl;
            return;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


        GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "LearnOpenGL Shader Exercise", nullptr, nullptr);

        if (window == nullptr)
        {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return;
        }

        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
            glfwTerminate();
            return;
        }

        // ———————————— 2. 创建顶点 —————————————
        float vertics[] = {
            0.5f , -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.0f , 0.5f , 0.0f, 0.0f, 0.0f, 1.0f
        };


        // ———————————— 3. 创建 VAO 和 VBO ————————————————
        unsigned int VAO = 0;
        unsigned int VBO = 0;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        // 先绑定 VAO，后续的顶点属性配置会记录到这个 VAO 里
        glBindVertexArray(VAO);

        // 绑定 VBO，并把顶点数据传入 GPU
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertics), vertics, GL_STATIC_DRAW);

        // location = 0，对应 vertex shader 里的 aPos
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            6 * sizeof(float),
            reinterpret_cast<void*>(0)
        );
        glEnableVertexAttribArray(0);

        // location = 1，对应 vertex shader 里的 aColor
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            6 * sizeof(float),
            reinterpret_cast<void*>(3 * sizeof(float))
        );
        glEnableVertexAttribArray(1);

        // ———————————— 4. 着色器选择 —————————————
        std::string vertexShaderPath;
        std::string fragmentShaderPath;
        switch (CURRENT_EXERCISE)
        {
            case ShaderExercise::FlipTriangle:
                vertexShaderPath = getShaderPath("exercise1.vert");
                fragmentShaderPath = getShaderPath("color.frag");
                break;

            case ShaderExercise::MoveTriangle:
                vertexShaderPath = getShaderPath("exercise2.vert");
                fragmentShaderPath = getShaderPath("color.frag");
                break;

            case ShaderExercise::PositionAsColor:
                vertexShaderPath = getShaderPath("exercise3.vert");
                fragmentShaderPath = getShaderPath("position_color.frag");
                break;
        }
        
        learnopengl::infrastructure::Shader shader(vertexShaderPath.c_str(), fragmentShaderPath.c_str());


        // —————————— 5. 显示 ————————————
        while (!glfwWindowShouldClose(window))
        {
            processInput(window);

            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            shader.use();

            // 练习 2：测试我们自己封装的 setFloat()
            if constexpr (CURRENT_EXERCISE == ShaderExercise::MoveTriangle)
            {
                float offset = 0.5f;
                shader.setFloat("xOffset", offset);
            }

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);

        glfwTerminate();

    }
}