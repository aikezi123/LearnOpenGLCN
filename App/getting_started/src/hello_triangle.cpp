#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

// 匿名命名空间：
// 这里面的函数和变量只在当前 .cpp 文件内部可见。
// 好处是：如果其他章节的 .cpp 文件里也有 processInput、framebuffer_size_callback，
// 不会发生链接阶段的重复定义错误。
namespace
{
    // 处理键盘输入
    //
    // 参数：
    // window：当前 GLFW 窗口对象指针。
    //         GLFW 会通过这个指针查询窗口状态、键盘状态，并控制窗口是否关闭。
    //
    // 作用：
    // 每一帧调用一次，检测 ESC 键是否被按下。
    // 如果 ESC 被按下，就告诉 GLFW：当前窗口应该关闭。
    void processInput(GLFWwindow* window)
    {
        // glfwGetKey(window, GLFW_KEY_ESCAPE)
        //
        // 参数 1：window
        //   要查询输入状态的窗口。
        //
        // 参数 2：GLFW_KEY_ESCAPE
        //   要查询的按键，这里是 ESC 键。
        //
        // 返回值：
        //   GLFW_PRESS   ：按键处于按下状态
        //   GLFW_RELEASE ：按键处于松开状态
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            // glfwSetWindowShouldClose(window, true)
            //
            // 参数 1：window
            //   要设置关闭状态的窗口。
            //
            // 参数 2：true
            //   表示希望关闭窗口。
            //
            // 作用：
            //   设置窗口的 should-close 标记。
            //   后面 while (!glfwWindowShouldClose(window)) 会检测这个标记。
            glfwSetWindowShouldClose(window, true);
        }
    }

    // 窗口大小变化时的回调函数
    //
    // 参数：
    // window：发生大小变化的 GLFW 窗口。
    // width ：新的窗口宽度。
    // height：新的窗口高度。
    //
    // 作用：
    // 当用户拖拽窗口改变大小时，GLFW 会自动调用这个函数。
    // 这里需要同步修改 OpenGL 的视口大小，否则渲染区域可能和窗口大小不一致。
    void framebuffer_size_callback(GLFWwindow* window, int width, int height)
    {
        // glViewport(x, y, width, height)
        //
        // 参数 1：x
        //   视口左下角的 x 坐标。这里是 0，表示从窗口最左侧开始。
        //
        // 参数 2：y
        //   视口左下角的 y 坐标。这里是 0，表示从窗口最底部开始。
        //
        // 参数 3：width
        //   视口宽度。
        //
        // 参数 4：height
        //   视口高度。
        //
        // 作用：
        //   告诉 OpenGL：最终渲染结果应该映射到窗口的哪一块区域。
        //   这里设置为整个窗口区域。
        glViewport(0, 0, width, height);
    }

    // 窗口初始宽度
    const unsigned int SCR_WIDTH = 800;

    // 窗口初始高度
    const unsigned int SCR_HEIGHT = 600;

    // 顶点着色器源码
    //
    // 顶点着色器的作用：
    //   处理每一个输入顶点。
    //   这里的顶点着色器只做一件事：
    //   把输入的顶点位置 aPos 直接赋值给 gl_Position。
    //
    // #version 330 core：
    //   表示使用 OpenGL 3.3 Core Profile 对应的 GLSL 版本。
    //
    // layout (location = 0) in vec3 aPos：
    //   表示输入变量 aPos 是一个 vec3。
    //   location = 0 表示它对应顶点属性位置 0。
    //   后面 glVertexAttribPointer(0, ...) 里的第一个参数 0 就和这里对应。
    //
    // gl_Position：
    //   OpenGL 内置变量，表示当前顶点最终的裁剪空间位置。
    const char* vertexShaderSource =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
        "}\0";

    // 片段着色器源码
    //
    // 片段着色器的作用：
    //   计算每一个片段，也可以简单理解成每一个待显示像素的颜色。
    //
    // out vec4 FragColor：
    //   输出变量，表示片段最终颜色。
    //
    // vec4(1.0f, 0.5f, 0.2f, 1.0f)：
    //   RGBA 颜色。
    //   R = 1.0，G = 0.5，B = 0.2，A = 1.0。
    //   最终显示为橙色。
    const char* fragmentShaderSource =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
        "}\n\0";
    }



// ——————————————————————————————————————————————————————
// ———————————————— OpenGL Context 理解 —————————————————
// OpenGL Context负责记录：当前用哪些对象和状态来渲染
//   Texture / VBO / VAO / Shader
//   是渲染过程中用到的资源和规则
//   Framebuffer
//   保存最终渲染出来的图像结果
//   OpenGL 可以理解为一套状态机式的图形 API。
//   程序当前使用的 OpenGL 状态由 OpenGL Context 管理。
//
// OpenGL Context 中有很多“绑定点/插槽”，例如：
//   GL_ARRAY_BUFFER         ：当前顶点属性缓冲绑定点
//   GL_ELEMENT_ARRAY_BUFFER ：当前索引缓冲绑定点
//   当前 VAO                 ：当前顶点数组对象
//   当前 Shader Program      ：当前使用的着色器程序
//   当前纹理绑定点            ：当前使用的纹理对象
//
// glGenBuffers 创建的是 OpenGL 缓冲对象 ID。
// glBindBuffer 并不会上传数据，它只是把某个缓冲对象设置为某个绑定点的“当前对象”。
// glBufferData 才会给当前绑定的缓冲对象分配存储，并把 CPU 数据复制进去。
//
// 可以近似理解为：
// OpenGL Context
// ├─ GL_ARRAY_BUFFER 当前绑定点       -> 当前绑定的 VBO
// ├─ GL_ELEMENT_ARRAY_BUFFER 当前绑定点 -> 当前绑定的 EBO
// ├─ 当前 VAO                         -> 当前使用的 VAO
// ├─ 当前 Shader Program              -> 当前使用的 Shader Program
// ├─ 当前纹理绑定点                    -> 当前使用的 Texture
// └─ 已创建的 OpenGL 对象
//    ├─ VBO 1
//    ├─ VBO 2
//    ├─ VAO 1
//    ├─ Shader Program 1
//    └─ Texture 1
//
// VBO 对象的数据存储由 OpenGL 驱动管理，通常位于 GPU 显存中。
// ——————————————————————————————————————————————————————

int hello_triangle()
{
    // —————————— 1. CPU内存中那个创建顶点坐标 ————————————
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f,  0.5f, 0.0f
    };

    // —————————— 2. 将CPU中的顶点数据上传到GPU ————————
    // ——————————— GPU绘制主要从显存里读取数据，而不是从CPU中读取数据，并且CPU内存里的数据生命周期可能很短，函数结束数组就失效了——————————
    // ——————————— GPU渲染是异步的。调用OpenGL绘图时，GPU可能稍后执行。如果GPU依赖CPU上的临时数据很不稳定。 ——————————
    // ——————————— 把数据一次性上传到GPU，后面重复绘制时就不需要每一帧都从CPU传一次，效率更高 —————————


    // 生成带有缓冲ID的顶点缓冲对象(VBO)
    unsigned int VBO;       // 生成编号
    glGenBuffers(1, &VBO);  // 创建一个缓冲对象，并把它的ID写入VBO。每一个顶点缓冲对象都有一个对应的ID。

    // 绑定顶点缓冲对象(VBO)绑定到OpenGL Context当前GL_ARRAY_BUFFER插槽上。
    glBindBuffer(GL_ARRAY_BUFFER, VBO);    // GL_ARRAY_BUFFER表示这个缓冲区将用来存储顶点属性数据
    
    // 把CPU的顶点数据上传到OpenGL Context当前GL_ARRAY_BUFFER绑定的顶点缓冲对象(VBO)上去
    // 参数 1：GL_ARRAY_BUFFER，操作当前绑定在 GL_ARRAY_BUFFER 上的缓冲对象，也就是 VBO。
    // 参数 2：sizeof(vertices)，要上传的数据大小，单位是字节。
    // 参数 3：vertices，CPU 内存中顶点数组的首地址。
    // 参数 4：GL_STATIC_DRAW，使用提示，表示数据基本不变，并且主要用于绘制。
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);  // 在GPU中给当前绑定的VBO分配内存，并把CPU中的vertices数据复制进去


    // —————————— 3. 顶点着色器 ————————————
    const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main() \n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0); \n"
    "}\0";
 

    return 0;
}

