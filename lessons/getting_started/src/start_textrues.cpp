#include "start_textrues.h"
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <stb_image.h>
#include <iostream>
#include <shader/Shader.hpp>


/*
 * ==================== OpenGL 双纹理绘制流程 ====================
 *
 * 本程序的目标：
 *
 * 1. 加载 container.jpg 和 awesomeface.png 两张图片；
 * 2. 将两张图片分别上传到两个 OpenGL 纹理对象；
 * 3. 将两个纹理对象绑定到不同的纹理单元；
 * 4. 在片段着色器中同时采样两张纹理；
 * 5. 使用 mix() 将两张纹理混合；
 * 6. 将最终渲染结果显示在 GLFW 创建的窗口中。
 *
 *
 * 一、GLFW 的作用
 *
 * GLFW 主要负责：
 *
 * - 创建操作系统窗口；
 * - 创建 OpenGL Context；
 * - 接收键盘、鼠标和窗口事件；
 * - 通过 glfwSwapBuffers() 把 OpenGL 渲染好的画面显示出来。
 *
 * GLFW 本身不负责加载、保存或采样纹理。
 * 纹理处理和图形绘制由 OpenGL 完成。
 *
 *
 * 二、图片是怎样进入 GPU 的
 *
 * 硬盘中的 JPG、PNG 文件是压缩图片，不能直接交给 OpenGL 使用。
 *
 * 第一步：stb_image 将图片解码到 CPU 内存
 *
 *     unsigned char* data = stbi_load(...);
 *
 * 数据流：
 *
 *     container.jpg / awesomeface.png
 *                  ↓ stbi_load
 *        CPU 内存中的原始像素数组 data
 *
 * 第二步：创建 OpenGL 纹理对象
 *
 *     glGenTextures(1, &textureID);
 *
 * textureID 是纹理对象的编号，可以理解为 GPU 纹理资源的句柄。
 *
 * 第三步：绑定纹理对象
 *
 *     glBindTexture(GL_TEXTURE_2D, textureID);
 *
 * 绑定后，接下来的 glTexParameteri()、glTexImage2D()、
 * glGenerateMipmap() 都会操作当前绑定的纹理对象。
 *
 * 第四步：将 CPU 像素上传到 GPU 纹理对象
 *
 *     glTexImage2D(..., data);
 *
 * 数据流：
 *
 *     CPU 像素数组 data
 *              ↓ glTexImage2D
 *     GPU 中的纹理对象 textureID
 *
 * 上传完成后，图片数据已经复制到 GPU，
 * 因此可以调用：
 *
 *     stbi_image_free(data);
 *
 * 释放 CPU 端的图片数据。
 *
 *
 * 三、纹理对象和纹理单元的区别
 *
 * 纹理对象：
 *
 *     textureID1
 *     textureID2
 *
 * 真正保存图片像素、尺寸、过滤方式、环绕方式和 Mipmap 数据。
 *
 * 纹理单元：
 *
 *     GL_TEXTURE0
 *     GL_TEXTURE1
 *     GL_TEXTURE2
 *     ...
 *
 * 纹理单元不永久保存图片。
 * 它是绘制时让着色器访问纹理对象的“中间插槽”。
 *
 * 例如：
 *
 *     glActiveTexture(GL_TEXTURE0);
 *     glBindTexture(GL_TEXTURE_2D, textureID1);
 *
 * 表示：
 *
 *     将 textureID1 绑定到纹理单元 0 的二维纹理位置。
 *
 * 再例如：
 *
 *     glActiveTexture(GL_TEXTURE1);
 *     glBindTexture(GL_TEXTURE_2D, textureID2);
 *
 * 表示：
 *
 *     将 textureID2 绑定到纹理单元 1 的二维纹理位置。
 *
 *
 * 四、片段着色器中的 sampler2D 是什么
 *
 * 片段着色器中：
 *
 *     uniform sampler2D texture1;
 *     uniform sampler2D texture2;
 *
 * sampler2D 本身不保存图片，也不保存 textureID。
 *
 * sampler2D 保存的是“纹理单元索引”。
 *
 * CPU 代码：
 *
 *     myShader.use();
 *     myShader.setInt("texture1", 0);
 *     myShader.setInt("texture2", 1);
 *
 * 表示：
 *
 *     texture1 的值为 0 → 使用纹理单元 GL_TEXTURE0
 *     texture2 的值为 1 → 使用纹理单元 GL_TEXTURE1
 *
 * 注意：
 *
 *     这里传入的是纹理单元索引 0、1，
 *     不是 textureID1、textureID2，
 *     也不是 GL_TEXTURE0、GL_TEXTURE1 枚举值。
 *
 *
 * 五、两张图片与片段着色器的完整关联关系
 *
 * 第一张纹理：
 *
 *     container.jpg
 *          ↓ stbi_load
 *     CPU 像素数据
 *          ↓ glTexImage2D
 *     textureID1
 *          ↓ glBindTexture
 *     GL_TEXTURE0
 *          ↑
 *     sampler2D texture1 的值为 0
 *
 * 第二张纹理：
 *
 *     awesomeface.png
 *          ↓ stbi_load
 *     CPU 像素数据
 *          ↓ glTexImage2D
 *     textureID2
 *          ↓ glBindTexture
 *     GL_TEXTURE1
 *          ↑
 *     sampler2D texture2 的值为 1
 *
 *
 * 六、片段着色器怎样读取纹理
 *
 * 片段着色器中：
 *
 *     texture(texture1, TexCoord)
 *
 * GPU 的查找过程：
 *
 * 1. texture1 的值是 0；
 * 2. 找到纹理单元 GL_TEXTURE0；
 * 3. sampler 类型是 sampler2D；
 * 4. 找到 GL_TEXTURE0 上绑定的二维纹理 textureID1；
 * 5. 根据 TexCoord 从 textureID1 中采样颜色。
 *
 * 第二张纹理同理：
 *
 *     texture(texture2, TexCoord)
 *
 * 会通过纹理单元 GL_TEXTURE1 找到 textureID2。
 *
 *
 * 七、两张纹理如何混合
 *
 * 片段着色器：
 *
 *     FragColor = mix(
 *         texture(texture1, TexCoord),
 *         texture(texture2, TexCoord),
 *         0.2
 *     );
 *
 * mix(a, b, 0.2) 的含义是：
 *
 *     最终颜色 = a * 80% + b * 20%
 *
 * 因此：
 *
 *     container.jpg 占 80%
 *     awesomeface.png 占 20%
 *
 * 这个混合结果不会生成新的图片文件，
 * 而是在每个片段执行片段着色器时实时计算，
 * 然后写入 OpenGL 帧缓冲。
 *
 *
 * 八、最终怎样显示到窗口
 *
 *     glDrawElements(...)
 *
 * 启动完整图形渲染管线：
 *
 *     顶点数据
 *         ↓
 *     顶点着色器
 *         ↓
 *     三角形组装
 *         ↓
 *     光栅化
 *         ↓
 *     插值纹理坐标
 *         ↓
 *     片段着色器采样并混合两张纹理
 *         ↓
 *     结果写入后缓冲
 *
 * 最后：
 *
 *     glfwSwapBuffers(window);
 *
 * 将 OpenGL 已经渲染好的后缓冲切换到前台，
 * 从而显示在 GLFW 窗口中。
 *
 *
 * 九、整个过程的简化总结
 *
 *     图片文件
 *         ↓ stb_image 解码
 *     CPU 像素数据
 *         ↓ glTexImage2D
 *     OpenGL 纹理对象
 *         ↓ glBindTexture
 *     纹理单元
 *         ↑ sampler2D 保存纹理单元索引
 *     片段着色器采样纹理
 *         ↓
 *     glDrawElements 生成最终画面
 *         ↓
 *     glfwSwapBuffers 显示到窗口
 *
 * ==============================================================
 */




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
    // ———————————— 4.1 纹理1 —————————————
    // 1. 生成纹理
    unsigned int textureID1;
    glGenTextures(1, &textureID1);
    glBindTexture(GL_TEXTURE_2D, textureID1);

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
    // 决定放大时的采样方式。设置为线性过滤。Mipmap只用于缩小，不用于放大。
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

    // ———————————— 4.2 纹理2 ——————————————
    unsigned int textureID2;
    glGenTextures(1, &textureID2);
    glBindTexture(GL_TEXTURE_2D, textureID2);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    std::string tex2Path = LEARNOPENGL_ASSET_DIR + std::string("/textures/getting_started/texture_texture/awesomeface.png");
    data = stbi_load(tex2Path.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    
    // ———————————— 5. 绘制图像 ———————————————
    std::string shaderPath = LEARNOPENGL_ASSET_DIR + std::string("/shaders/getting_started/shader_texture");
    std::string vertexShaderPath = shaderPath + "/texture.vert";
    std::string fragShaderPath = shaderPath + "/texture.frag";
    learnopengl::infrastructure::Shader myShader(vertexShaderPath.c_str(), fragShaderPath.c_str());

    myShader.use();

    // 设置纹理单元索引。texture1 的值是 0 → 使用纹理单元 GL_TEXTURE0
    myShader.setInt("texture1", 0);
    // 设置纹理单元索引。texture2 的值是 1 → 使用纹理单元 GL_TEXTURE1
    myShader.setInt("texture2", 1);

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 绘制前把纹理对象绑定到纹理单元
        // GL_TEXTURE0 的二维纹理绑定位置 → textureID1
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID1);
        // GL_TEXTURE1 的二维纹理绑定位置 → textureID2
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureID2);

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
