#include "start_shaders.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

int start_shaders() {
    // ———————— 1. 创建GLFW窗口并绑定OpenGL Context上下文 ————————
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(1920, 1080, "LearnOpenGL", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height) {
        glViewport(0, 0, 1920, 1080);
    });

    // ———————— 2. 初始化GLAD ——————————
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "failed to initialize GLAD" << std::endl;
        return -1;
    }


    // ———————— 3. 顶点着色器 ——————————
    // —————————— 3.1 unifrom版本着色器 ——————————
    const char *vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";
    unsigned int vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderID, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShaderID);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShaderID, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED" << infoLog << std::endl;
    }

    // —————————— 3.2 ourColor版本着色器
    const char *vertexShaderSource2 = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;      // 位置变量的属性位置为0
        layout(location = 1) in vec3 aColor;    // 颜色变量的属性位置为1
        out vec3 ourColor;                      // 向片段着色器输出一个颜色

        void main() {
            gl_Position = vec4(aPos, 1.0);     
            ourColor = aColor;                  // 将ourColor设置为我们从顶点数据那里得到的输入颜色
        }
    )";
    unsigned int vertexShaderID2 = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderID2, 1, &vertexShaderSource2, nullptr);
    glCompileShader(vertexShaderID2);
    glGetShaderiv(vertexShaderID2, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShaderID2, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED" << infoLog << std::endl;
    }
    
    // ————————— 4. 片段着色器 ——————————
    // ————————— 4.1 unifrom版本着色器 —————————— 
    const char *fragShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec4 outColor;

        void main() {
            FragColor = outColor;
        }
    )"; 

    unsigned int fragShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShaderID, 1, &fragShaderSource, nullptr);
    glCompileShader(fragShaderID);

    glGetShaderiv(fragShaderID, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragShaderID, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::FRAG::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // —————————— 4.2 ourColor版本着色器 ————————————
    const char *fragShaderSource2 = R"(
        #version 330 core
        out vec4 FragColor;
        in vec3 ourColor;

        void main() {
            FragColor = vec4(ourColor, 1.0);
        }
    )"; 
    unsigned int fragShaderID2 = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShaderID2, 1, &fragShaderSource2, nullptr);
    glCompileShader(fragShaderID2);

    glGetShaderiv(fragShaderID2, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragShaderID2, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::FRAG::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // —————————— 5. 编译链接 ——————————
    // —————————— 5.1 unifrom版本着色器链接 ————————————
    unsigned int shaderProgramID = glCreateProgram();
    glAttachShader(shaderProgramID, vertexShaderID);
    glAttachShader(shaderProgramID, fragShaderID);
    glLinkProgram(shaderProgramID);

    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgramID, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragShaderID);

    // —————————— 5.2 ourColor版着色器链接 ————————————
    unsigned int shaderProgramID2 = glCreateProgram();
    glAttachShader(shaderProgramID2, vertexShaderID2);
    glAttachShader(shaderProgramID2, fragShaderID2);
    glLinkProgram(shaderProgramID2);
    glGetProgramiv(shaderProgramID2, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgramID2, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShaderID2);
    glDeleteShader(fragShaderID2);
    
    // —————————— 6. 创建顶点 ——————————
    // —————————— 6.1 unifrom版本顶点 ————————————
    float vertics[] = {
         0.5f, -0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    // —————————— 6.2 ourColor版本顶点 ————————————
    float vertics2[] = {
    // 位置              // 颜色
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // 右下
    -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // 左下
     0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // 顶部
    };
    

    // —————————— 7. 创建VAO、VBO ——————————
    // —————————— 7.1 unifrom版本VAO、VBO ——————————
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertics), vertics, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(VAO);

    // ———————————— 7.2 ourColor版本VAO、VBO ————————————
    unsigned int VBO2, VAO2;
    glGenBuffers(1, &VBO2);
    glBindBuffer(GL_ARRAY_BUFFER, VBO2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertics2), vertics2, GL_STATIC_DRAW);
    glGenVertexArrays(1, &VAO2);
    glBindVertexArray(VAO2);


    // 位置信息分配
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 颜色信息分配
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(VAO2);

    // —————————— 8. 循环绘图 ————————————
    glUseProgram(shaderProgramID2);
    // —————————— 8.1 unifrom版本绘图（随时间变色）——————————
    // while (!glfwWindowShouldClose(window)) {
    //     if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    //         glfwSetWindowShouldClose(window, true);
    //     }
    //     // 设置清屏颜色。
    //     // 这里只是设置“之后清屏时要使用什么颜色”，还没有真正清屏。
    //     glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            
    //     // 清空颜色缓冲区。
    //     // 也就是用上面 glClearColor 设置的颜色，把当前 framebuffer 的颜色内容清掉。
    //     // 每一帧绘制前通常都要清屏，否则上一帧的内容可能残留。
    //     glClear(GL_COLOR_BUFFER_BIT);
            
            
    //     // 使用 shader program。
    //     //
    //     // shaderProgramID 是前面由：
    //     //   顶点着色器 + 片段着色器
    //     // 链接生成的完整 GPU 程序。
    //     //
    //     // 调用 glUseProgram(shaderProgramID) 后，
    //     // OpenGL Context 当前使用的 shader program 就变成 shaderProgramID。
    //     //
    //     // 注意：
    //     //   glUniform... 设置 uniform 变量时，默认作用于“当前正在使用的 shader program”。
    //     //   所以一般要先 glUseProgram(shaderProgramID)，再设置 uniform。
    //     glUseProgram(shaderProgramID);
            
            
    //     // 获取当前程序运行时间。
    //     //
    //     // glfwGetTime() 返回从 GLFW 初始化以来经过的时间，单位是秒。
    //     // 这里用时间值来动态改变颜色，让三角形颜色随时间变化。
    //     double timeValue = glfwGetTime();
            
            
    //     // 计算绿色分量。
    //     //
    //     // sin(timeValue) 的范围是 [-1, 1]。
    //     //
    //     // sin(timeValue) / 2.0 的范围是 [-0.5, 0.5]。
    //     // 再加上 0.5 后，范围变成 [0.0, 1.0]。
    //     //
    //     // 这样 greenValue 就可以作为颜色分量使用。
    //     // OpenGL 颜色分量通常使用 0.0 到 1.0。
    //     float greenValue = static_cast<float>(sin(timeValue) / 2.0 + 0.5);
            
            
    //     // 查询片段着色器中的 uniform 变量位置。
    //     //
    //     // 你的片段着色器中应该有类似代码：
    //     //
    //     //   uniform vec4 outColor;
    //     //
    //     //   void main()
    //     //   {
    //     //       FragColor = outColor;
    //     //   }
    //     //
    //     // uniform 的作用：
    //     //   uniform 是 C++ 程序传给 shader 的“统一参数”。
    //     //   它不是来自 VBO，也不是每个顶点不同的数据。
    //     //   在一次 draw call 中，所有顶点或片段通常看到的是同一个 uniform 值。
    //     //
    //     // glGetUniformLocation(program, name)
    //     //
    //     // 参数 1：shaderProgramID
    //     //   要查询哪个 shader program 中的 uniform 变量。
    //     //
    //     // 参数 2："outColor"
    //     //   要查询的 uniform 变量名。
    //     //   这个名字必须和 shader 源码中的 uniform 名字完全一致。
    //     //
    //     // 返回值：
    //     //   返回 outColor 在 shaderProgramID 中的 uniform location。
    //     //   可以把它理解为 OpenGL 给这个 uniform 变量分配的编号/句柄。
    //     //
    //     // 注意：
    //     //   这个 location 不是 C++ 内存地址。
    //     //   它只是 OpenGL 内部用于定位 shader uniform 变量的编号。
    //     //
    //     // 如果返回 -1，通常表示：
    //     //   1. uniform 名字写错了；
    //     //   2. shader 中声明了但没有实际使用，被编译器优化掉了；
    //     //   3. shader program 链接失败。
    //     int vertexColorLocation = glGetUniformLocation(shaderProgramID, "outColor");
            
            
    //     // 给 shader 中的 uniform vec4 outColor 赋值。
    //     //
    //     // glUniform4f(location, v0, v1, v2, v3)
    //     //
    //     // 参数 1：vertexColorLocation
    //     //   要设置的 uniform 变量位置。
    //     //   这里对应 shader 中的 outColor。
    //     //
    //     // 参数 2：0.0f
    //     //   outColor 的 R 分量，红色。
    //     //
    //     // 参数 3：greenValue
    //     //   outColor 的 G 分量，绿色。
    //     //   这里会随时间在 0.0 到 1.0 之间变化。
    //     //
    //     // 参数 4：0.0f
    //     //   outColor 的 B 分量，蓝色。
    //     //
    //     // 参数 5：1.0f
    //     //   outColor 的 A 分量，透明度。
    //     //   1.0 表示完全不透明。
    //     //
    //     // 因为 shader 中 outColor 是 vec4，
    //     // 所以这里使用 glUniform4f 传入 4 个 float。
    //     //
    //     // 设置完成后，片段着色器执行到：
    //     //
    //     //   FragColor = outColor;
    //     //
    //     // 时，FragColor 就会使用这里传入的颜色。
    //     glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);
            
            
    //     // 发起绘制命令。
    //     //
    //     // glDrawArrays(GL_TRIANGLES, 0, 3)
    //     //
    //     // 参数 1：GL_TRIANGLES
    //     //   每 3 个顶点组成一个三角形。
    //     //
    //     // 参数 2：0
    //     //   从第 0 个顶点开始读取。
    //     //
    //     // 参数 3：3
    //     //   读取 3 个顶点。
    //     //
    //     // 执行这句时，GPU 才真正运行当前 shader program：
    //     //   顶点着色器处理顶点位置；
    //     //   光栅化阶段生成片段；
    //     //   片段着色器读取 uniform outColor，输出片段颜色。
    //     glDrawArrays(GL_TRIANGLES, 0, 3);
            
            
    //     // 交换前后缓冲。
    //     // OpenGL 通常先画到后缓冲，调用 glfwSwapBuffers 后，后缓冲内容显示到窗口。
    //     glfwSwapBuffers(window);
            
            
    //     // 处理窗口事件。
    //     // 包括键盘、鼠标、窗口关闭、窗口大小变化等。
    //     glfwPollEvents();
    // }

    // glDeleteVertexArrays(1, &VAO);
    // glDeleteBuffers(1, &VBO);
    // glDeleteProgram(shaderProgramID);

    
    // ————————————8.2 ourColor版本绘图 ————————————
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }
        
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO2);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO2);
    glDeleteBuffers(1, &VBO2);
    glDeleteProgram(shaderProgramID2);
    glfwTerminate();
}