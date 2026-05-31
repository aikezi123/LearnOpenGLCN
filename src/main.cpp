#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // OpenGL 需要知道你想在窗口的哪一部分进行绘制。
    // glViewport(x, y, width, height) 告诉 OpenGL：“请把我渲染出来的画面，映射到从窗口左下角 (0, 0) 开始，宽为 width、高为 height 的区域内。"
    // 如果没有这行代码，当你拉伸窗口时，里面的 3D 画面比例就不会跟着变化，或者只能在一个固定的小角落里渲染。
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
    // 主动向系统查询——“此时此刻，ESC 键（GLFW_KEY_ESCAPE）是不是正处于被按下的状态（GLFW_PRESS）？”
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        // 如果检测到按下了 ESC 键，就给当前的窗口打上一个“该关门了”的标记（设置为 true）。
            glfwSetWindowShouldClose(window, true);
    }
}

int main()
{
    // 初始化GLFW库
    glfwInit();
    // 指定所需的 OpenGL 版本号。我们希望即将创建的 OpenGL 上下文版本是 3.3（主版本号 Major 为 3，次版本号 Minor 为 3）
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // 启用 OpenGL 的核心模式（Core-profile）
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    // 创建一个窗口
    // 作用： 调用 GLFW 的函数实例化一个真实的操作系统窗口，同时也会为这个窗口创建一个 OpenGL 上下文（Context）。
    GLFWwindow *window = glfwCreateWindow(800, 600, "LearnOpenGL", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // 作用： 将我们刚刚创建的窗口的上下文（Context）设置为当前线程的主上下文。
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);


    // GLAD 是用来管理 OpenGL 函数指针的库，我们需要在调用任何 OpenGL 函数之前初始化 GLAD。
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }    


    // 渲染主循环 
    while (!glfwWindowShouldClose(window))
    {
        // 1. 处理输入
        processInput(window);

        // 设置清空屏幕时所使用的底色。这属于一个状态设置函数,这行代码并不会真正去清空屏幕。
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        // 真正执行清空屏幕的动作。这属于一个状态使用函数。
        glClear(GL_COLOR_BUFFER_BIT);
        // 2. 交换双缓冲
        // 应用程序使用单缓冲绘图时可能会存在图像闪烁的问题。 这是因为生成的图像不是一下子被绘制出来的，而是按照从左到右，由上而下逐像素地绘制而成的。
        // 最终图像不是在瞬间显示给用户，而是通过一步一步生成的，这会导致渲染的结果很不真实。为了规避这些问题，我们应用双缓冲渲染窗口应用程序。
        // 前缓冲保存着最终输出的图像，它会在屏幕上显示；而所有的的渲染指令都会在后缓冲上绘制。当所有的渲染指令执行完毕后，我们交换(Swap)前缓冲和后缓冲，这样图像就立即呈显出来，之前提到的不真实感就消除了。
        glfwSwapBuffers(window);

        // 3. 处理系统事件
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

