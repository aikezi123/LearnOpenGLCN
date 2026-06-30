#include <DisplayOpenGLImage.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <stb_image.h>

#include <array>
#include <iostream>
#include <string>
#include <utility>

namespace {

std::string findTexturePath()
{
    const QString relativePath = QStringLiteral("assets/textures/ui/display_image.png");
    const QDir currentDir = QDir::current();
    const QDir appDir = QDir(QCoreApplication::applicationDirPath());

    const std::array<QString, 3> candidates{
        currentDir.absoluteFilePath(relativePath),
        appDir.absoluteFilePath(QStringLiteral("../../../../") + relativePath),
        appDir.absoluteFilePath(QStringLiteral("../") + relativePath)
    };

    for (const QString& candidate : candidates) {
        const QString cleanPath = QDir::cleanPath(candidate);
        if (QFileInfo::exists(cleanPath)) {
            const QByteArray encodedPath = QDir::toNativeSeparators(cleanPath).toLocal8Bit();
            return std::string(encodedPath.constData(), static_cast<size_t>(encodedPath.size()));
        }
    }

    return {};
}

// 编译着色器，返回着色器ID
GLuint compileShader(QOpenGLFunctions_3_3_Core& gl, GLenum type, const char* source)
{
    // 根据type类型创建着色器对象，并返回它的OpenGL ID
    const GLuint shaderID = gl.glCreateShader(type);
    // 把GLSL着色器源码绑定到shaderID对应的着色器对象上
    gl.glShaderSource(shaderID, 1, &source, nullptr);
    // 编译ShaderID对应的GLSL源码
    gl.glCompileShader(shaderID);

    GLint success = 0;
    // 查询shaderID的某个整数状态
    gl.glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);

    if (!success) {
        char infoLog[512]{};
        // 获取Shader编译失败时的错误日志
        gl.glGetShaderInfoLog(shaderID, sizeof(infoLog), nullptr, infoLog);

        std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;

        // 删除着色器对象，释放OpenGL资源
        gl.glDeleteShader(shaderID);
        return 0;
    }

    return shaderID;
}

} // namespace

DisplayOpenGLImage::DisplayOpenGLImage(QWidget *parent) : QOpenGLWidget(parent) {
}

DisplayOpenGLImage::~DisplayOpenGLImage()
{
    if (context() != nullptr) {
        makeCurrent();
        cleanup();
        doneCurrent();
    }
}

void DisplayOpenGLImage::setRgb24Frame(int width, int height, std::vector<unsigned char> pixels)
{
    if (width <= 0 || height <= 0 || pixels.empty()) {
        return;
    }

    m_pendingFrameWidth = width;
    m_pendingFrameHeight = height;
    m_pendingRgb24Frame = std::move(pixels);
    m_hasPendingCameraFrame = true;

    update();
}

void DisplayOpenGLImage::initializeGL()
{
    initializeOpenGLFunctions();   // 初始化opengl函数库

    initializeShader();            // 初始化着色器
    initializeGeometry();          // 初始化几何对象
    initializeTexture();           // 初始化纹理

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
}

void DisplayOpenGLImage::resizeGL(int width, int height) {
    // 从当前framebuffer的左下角开始。使用宽度width、高度height的矩形区域作为OpenGL的绘制区域
    glViewport(0, 0, width, height);
}

void DisplayOpenGLImage::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    uploadPendingCameraFrame();

    if (m_shaderProgram == 0 || m_texture == 0 || m_vao == 0) {
        return;
    }
    
    glUseProgram(m_shaderProgram);

    // 激活纹理单元 0。
    glActiveTexture(GL_TEXTURE0);

    // 把 m_texture 绑定到当前 GL_TEXTURE_2D。
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

void DisplayOpenGLImage::initializeShader()
{
    // 着色器与顶点的关系
    // Shader 声明：
    // 我需要 location 0 的 vec3 aPos。
 
    // VAO 记录：
    // location 0 从 m_vbo 读取；
    // 每次读取 3 个 float；
    // 从 offset 0 开始；
    // 每个顶点间隔 3 * sizeof(float)。
 
    // VBO 提供：
    // 真实顶点坐标数据。
 
    // EBO 提供：
    // 按什么索引顺序取顶点。


    // 顶点着色器
    const char *vertexShaderSource = R"(
        #version 330 core
        
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aTexCoord;
        
        out vec2 TexCoord;
        
        void main()
        {
            gl_Position = vec4(aPos, 1.0);
            TexCoord = aTexCoord;
        }
    )";

    // 片段着色器
    const char *fragShaderSource = R"(
        #version 330 core

        in vec2 TexCoord;

        layout(location = 0) out vec4 fragColor;

        uniform sampler2D uTexture;

        void main()
        {
            fragColor = texture(uTexture, TexCoord);
        }
    )";

    // 编译顶点着色器
    unsigned int vertexShaderID = compileShader(*this, GL_VERTEX_SHADER, vertexShaderSource);
    // 编译片段着色器
    unsigned int fragShaderID = compileShader(*this, GL_FRAGMENT_SHADER, fragShaderSource);

    if (vertexShaderID == 0 || fragShaderID == 0) {
        if (vertexShaderID != 0) {
            glDeleteShader(vertexShaderID);
        }
        if (fragShaderID != 0) {
            glDeleteShader(fragShaderID);
        }
        return;
    }
    // 创建着色器程序
    m_shaderProgram = glCreateProgram();
    // 将着色器连接到程序中
    glAttachShader(m_shaderProgram, vertexShaderID);
    glAttachShader(m_shaderProgram, fragShaderID);
    glLinkProgram(m_shaderProgram);

    // 判断程序是否连接成功
    int success = 0;
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);

    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_shaderProgram, 512, nullptr, infoLog);

        std::cerr << "Shader program link failed:\n" << infoLog << std::endl;

        // 删除着色器程序
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
    }
    else {
        glUseProgram(m_shaderProgram);

        const GLint textureLocation =
            glGetUniformLocation(m_shaderProgram, "uTexture");

        if (textureLocation != -1) {
            glUniform1i(textureLocation, 0);
        }

        glUseProgram(0);
    }

    // link 成功后，m_shaderProgram 已经保存了完整的链接结果。
    // vertexShaderID 和 fragShaderID 只是中间 Shader Object.
    // 后续 paintGL() 只需要 glUseProgram(m_shaderProgram)，不再需要单独的 Shader Object。
    glDeleteShader(vertexShaderID);
    glDeleteShader(fragShaderID);
}

void DisplayOpenGLImage::initializeGeometry()
{
    /*
        initializeGeometry() 的作用：

        1. CPU 先准备矩形的顶点数据 vertices。
        2. CPU 先准备矩形的索引数据 indices。
        3. 通过 glGenVertexArrays / glGenBuffers 向 OpenGL Context 申请 VAO / Buffer 对象 ID。
        4. 通过 glBindVertexArray 绑定当前 VAO，开始配置这个 VAO。
        5. 通过 glBindBuffer(GL_ARRAY_BUFFER, m_vbo) 绑定顶点缓冲。
        6. 通过 glBufferData(GL_ARRAY_BUFFER, ...) 把 CPU 顶点数据交给当前绑定的 VBO。
            数据交给了 OpenGL 驱动管理的 Buffer Object。
            驱动决定它最终放在 CPU 侧驱动内存、GPU 显存，还是 GPU 可访问的共享内存中。
        7. 通过 glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo) 绑定索引缓冲。
           注意：EBO 绑定关系会被当前 VAO 记录。
        8. 通过 glBufferData(GL_ELEMENT_ARRAY_BUFFER, ...) 把 CPU 索引数据交给当前绑定的 EBO。
        9. 通过 glVertexAttribPointer 配置 shader 中 location = 0 的顶点属性如何从 VBO 中读取。
        10. 通过 glEnableVertexAttribArray(0) 启用 location = 0。
        11. 解绑 GL_ARRAY_BUFFER 和 VAO，避免后续误修改。

        最终结果：

        VAO m_vao 记录：
            location 0 从 m_vbo 读取；
            每个顶点读取 3 个 float；
            stride = 3 * sizeof(float)；
            offset = 0；
            EBO = m_ebo。

        VBO m_vbo 保存：
            4 个顶点的位置数据。

        EBO m_ebo 保存：
            6 个顶点索引，用于绘制两个三角形。

        后续 paintGL() 中只需要：

            glBindVertexArray(m_vao);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        OpenGL 就能根据 m_vao 中记录的配置找到 m_vbo 和 m_ebo，绘制矩形。
    */


    // CPU 准备顶点数组。
    //
    // 这里定义了 4 个顶点，每个顶点 3 个 float：x、y、z。
    //
    // 顶点 0：右上
    // 顶点 1：右下
    // 顶点 2：左下
    // 顶点 3：左上
    //
    // 几何形状：
    //
    //      3 -------- 0
    //      |          |
    //      |          |
    //      2 -------- 1
    //
    // 当前阶段：
    //
    // CPU:
    //     vertices 是 CPU 栈上的局部数组。
    //
    // OpenGL Context:
    //     暂时没有变化。
    //
    // GPU:
    //     暂时没有变化。
    const std::array<float, 20> vertices{
        // positions          // texture coordinates
         1.0F,  1.0F, 0.0F,   1.0F, 1.0F,
         1.0F, -1.0F, 0.0F,   1.0F, 0.0F,
        -1.0F, -1.0F, 0.0F,   0.0F, 0.0F,
        -1.0F,  1.0F, 0.0F,   0.0F, 1.0F
    };

    /*
        当前 OpenGL Context 概念状态表：

        对象表：
            暂无本函数创建的 VAO / Buffer 对象。

        当前绑定状态：
            current VAO = 0 或之前遗留状态
            current GL_ARRAY_BUFFER = 0 或之前遗留状态
            current GL_ELEMENT_ARRAY_BUFFER = 取决于 current VAO
            current shader program = 不受本函数影响
    */


    // CPU 准备索引数组。
    //
    // OpenGL 主要绘制三角形。
    // 这里用两个三角形拼出一个矩形：
    //
    // 第一个三角形：0, 1, 3
    // 第二个三角形：1, 2, 3
    //
    // CPU:
    //     indices 是 CPU 栈上的局部数组。
    //
    // OpenGL Context:
    //     暂时没有变化。
    //
    // GPU:
    //     暂时没有变化。
    const std::array<unsigned int, 6> indices{
        0, 1, 3,
        1, 2, 3
    };

    /*
        当前 OpenGL Context 概念状态表：

        对象表：
            暂无本函数创建的 VAO / Buffer 对象。

        当前绑定状态：
            current VAO = 0 或之前遗留状态
            current GL_ARRAY_BUFFER = 0 或之前遗留状态
            current GL_ELEMENT_ARRAY_BUFFER = 取决于 current VAO
            current shader program = 不受本函数影响
    */


    // 创建 1 个 VAO 对象 ID，保存到 m_vao。
    //
    // glGenVertexArrays 的第一个参数 1 表示生成 1 个 VAO ID。
    // &m_vao 表示把生成的 ID 写入 m_vao。
    //
    // CPU:
    //     m_vao 得到一个 GLuint ID，例如 m_vao = 1。
    //
    // OpenGL Driver / Context:
    //     在当前 Context 的 VAO 对象命名空间中生成一个 VAO 对象名。
    //
    // GPU:
    //     通常还没有绘制动作，也不一定分配大量 GPU 内存。
    glGenVertexArrays(1, &m_vao);

    /*
        当前 OpenGL Context 概念状态表：

        对象表：
            VAO ID m_vao -> VAO 对象
                location 配置：尚未设置
                element array buffer：尚未设置

        当前绑定状态：
            current VAO = 0 或之前遗留状态
            current GL_ARRAY_BUFFER = 0 或之前遗留状态
            current GL_ELEMENT_ARRAY_BUFFER = 取决于 current VAO
            current shader program = 不受本函数影响
    */


    // 创建 1 个 Buffer Object ID，保存到 m_vbo。
    //
    // 注意：
    //     glGenBuffers 创建的是通用 Buffer Object。
    //     它此时还不是严格意义上的 VBO。
    //
    // 后面当它绑定到 GL_ARRAY_BUFFER 后，才作为 VBO 使用。
    //
    // CPU:
    //     m_vbo 得到一个 GLuint ID，例如 m_vbo = 2。
    //
    // OpenGL Driver / Context:
    //     在 Buffer 对象命名空间中生成一个 Buffer 对象名。
    //
    // GPU:
    //     还没有真正的顶点数据。
    glGenBuffers(1, &m_vbo);

    /*
        当前 OpenGL Context 概念状态表：

        对象表：
            VAO ID m_vao -> VAO 对象
                location 配置：尚未设置
                element array buffer：尚未设置

            Buffer ID m_vbo -> Buffer 对象
                用途：尚未确定
                数据：尚未分配 / 尚未上传

        当前绑定状态：
            current VAO = 0 或之前遗留状态
            current GL_ARRAY_BUFFER = 0 或之前遗留状态
            current GL_ELEMENT_ARRAY_BUFFER = 取决于 current VAO
            current shader program = 不受本函数影响
    */


    // 创建 1 个 Buffer Object ID，保存到 m_ebo。
    //
    // 注意：
    //     m_ebo 本质上也是 Buffer Object。
    //     后面绑定到 GL_ELEMENT_ARRAY_BUFFER 后，才作为 EBO 使用。
    //
    // CPU:
    //     m_ebo 得到一个 GLuint ID，例如 m_ebo = 3。
    //
    // OpenGL Driver / Context:
    //     在 Buffer 对象命名空间中生成另一个 Buffer 对象名。
    //
    // GPU:
    //     还没有真正的索引数据。
    glGenBuffers(1, &m_ebo);

    /*
        当前 OpenGL Context 概念状态表：

        对象表：
            VAO ID m_vao -> VAO 对象
                location 配置：尚未设置
                element array buffer：尚未设置

            Buffer ID m_vbo -> Buffer 对象
                用途：尚未确定，后续作为 VBO 使用
                数据：尚未分配 / 尚未上传

            Buffer ID m_ebo -> Buffer 对象
                用途：尚未确定，后续作为 EBO 使用
                数据：尚未分配 / 尚未上传

        当前绑定状态：
            current VAO = 0 或之前遗留状态
            current GL_ARRAY_BUFFER = 0 或之前遗留状态
            current GL_ELEMENT_ARRAY_BUFFER = 取决于 current VAO
            current shader program = 不受本函数影响
    */


    // 绑定 VAO。
    //
    // 从这句开始，后续顶点属性配置和 EBO 绑定关系会记录到 m_vao 中。
    //
    // CPU:
    //     调用 OpenGL API。
    //
    // OpenGL Driver / Context:
    //     修改当前绑定状态：
    //         current VAO = m_vao
    //
    // GPU:
    //     暂时没有绘制动作。
    glBindVertexArray(m_vao);

    /*
        当前 OpenGL Context 概念状态表：

        对象表：
            VAO ID m_vao -> VAO 对象
                location 配置：尚未设置
                element array buffer：尚未设置

            Buffer ID m_vbo -> Buffer 对象
                用途：尚未确定，后续作为 VBO 使用
                数据：尚未分配 / 尚未上传

            Buffer ID m_ebo -> Buffer 对象
                用途：尚未确定，后续作为 EBO 使用
                数据：尚未分配 / 尚未上传

        当前绑定状态：
            current VAO = m_vao
            current GL_ARRAY_BUFFER = 0 或之前遗留状态
            current GL_ELEMENT_ARRAY_BUFFER = 当前 VAO m_vao 中尚未设置
            current shader program = 不受本函数影响
    */


    // 绑定 m_vbo 到 GL_ARRAY_BUFFER。
    //
    // 这句表示：
    //     当前 GL_ARRAY_BUFFER 绑定点使用 m_vbo。
    //
    // 后续调用：
    //     glBufferData(GL_ARRAY_BUFFER, ...)
    //
    // 就会把数据交给 m_vbo。
    //
    // CPU:
    //     调用 OpenGL API。
    //
    // OpenGL Driver / Context:
    //     修改当前绑定状态：
    //         current GL_ARRAY_BUFFER = m_vbo
    //
    // GPU:
    //     暂时没有绘制动作。
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    /*
        当前 OpenGL Context 概念状态表：

        对象表：
            VAO ID m_vao -> VAO 对象
                location 配置：尚未设置
                element array buffer：尚未设置

            Buffer ID m_vbo -> Buffer 对象
                当前绑定目标：GL_ARRAY_BUFFER
                数据：尚未分配 / 尚未上传

            Buffer ID m_ebo -> Buffer 对象
                用途：尚未确定，后续作为 EBO 使用
                数据：尚未分配 / 尚未上传

        当前绑定状态：
            current VAO = m_vao
            current GL_ARRAY_BUFFER = m_vbo
            current GL_ELEMENT_ARRAY_BUFFER = 当前 VAO m_vao 中尚未设置
            current shader program = 不受本函数影响
    */


    // 把 CPU 端 vertices 数据交给当前 GL_ARRAY_BUFFER 绑定的 Buffer。
    //
    // 因为前面已经：
    //     glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    //
    // 所以这里实际操作的是 m_vbo。
    //
    // 参数说明：
    //     GL_ARRAY_BUFFER:
    //         操作当前顶点数组缓冲绑定点。
    //
    //     vertices.size() * sizeof(float):
    //         顶点数据总字节数。
    //
    //     vertices.data():
    //         CPU 端顶点数组首地址。
    //
    //     GL_STATIC_DRAW:
    //         usage hint，告诉驱动这份数据基本不变，主要用于绘制。
    //
    // CPU:
    //     把 vertices 的地址和大小传给 OpenGL。
    //
    // OpenGL Driver / Context:
    //     为 m_vbo 对应的 Buffer Object 分配/管理存储；
    //     接收 vertices 顶点数据。
    //
    // GPU:
    //     数据通常会被驱动安排到 GPU 可高效访问的内存中。
    //     但具体是否立刻进入显存，由驱动决定。
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
        vertices.data(),
        GL_STATIC_DRAW
    );

    /*
        当前 OpenGL Context 概念状态表：

        对象表：
            VAO ID m_vao -> VAO 对象
                location 配置：尚未设置
                element array buffer：尚未设置

            Buffer ID m_vbo -> Buffer 对象
                当前绑定目标：GL_ARRAY_BUFFER
                保存数据：
                    vertices 顶点数据
                    顶点 0:  0.5,  0.5, 0.0
                    顶点 1:  0.5, -0.5, 0.0
                    顶点 2: -0.5, -0.5, 0.0
                    顶点 3: -0.5,  0.5, 0.0

            Buffer ID m_ebo -> Buffer 对象
                用途：尚未确定，后续作为 EBO 使用
                数据：尚未分配 / 尚未上传

        当前绑定状态：
            current VAO = m_vao
            current GL_ARRAY_BUFFER = m_vbo
            current GL_ELEMENT_ARRAY_BUFFER = 当前 VAO m_vao 中尚未设置
            current shader program = 不受本函数影响
    */


    // 绑定 m_ebo 到 GL_ELEMENT_ARRAY_BUFFER。
    //
    // 这句表示：
    //     当前索引缓冲绑定点使用 m_ebo。
    //
    // 注意：
    //     当前 m_vao 正在绑定。
    //     因此 GL_ELEMENT_ARRAY_BUFFER 的绑定关系会被当前 VAO 记录。
    //
    // 也就是说，m_vao 会记住：
    //     element array buffer = m_ebo
    //
    // CPU:
    //     调用 OpenGL API。
    //
    // OpenGL Driver / Context:
    //     修改当前 VAO m_vao 的状态：
    //         m_vao.elementArrayBuffer = m_ebo
    //
    // GPU:
    //     暂时没有绘制动作。
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    /*
        当前 OpenGL Context 概念状态表：

        对象表:
            VAO ID m_vao -> VAO 对象
                location 配置：尚未设置
                element array buffer = m_ebo

            Buffer ID m_vbo -> Buffer 对象
                保存数据：
                    vertices 顶点数据

            Buffer ID m_ebo -> Buffer 对象
                当前绑定目标：GL_ELEMENT_ARRAY_BUFFER
                数据：尚未分配 / 尚未上传

        当前绑定状态：
            current VAO = m_vao
            current GL_ARRAY_BUFFER = m_vbo
            current GL_ELEMENT_ARRAY_BUFFER = m_ebo
                注意：这个绑定关系属于当前 VAO m_vao 的状态
            current shader program = 不受本函数影响
    */


    // 把 CPU 端 indices 数据交给当前 GL_ELEMENT_ARRAY_BUFFER 绑定的 Buffer。
    //
    // 因为前面已经：
    //     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    //
    // 所以这里实际操作的是 m_ebo。
    //
    // CPU:
    //     把 indices 的地址和大小传给 OpenGL。
    //
    // OpenGL Driver / Context:
    //     为 m_ebo 对应的 Buffer Object 分配/管理存储；
    //     接收 indices 索引数据。
    //
    // GPU:
    //     索引数据通常会被驱动安排到 GPU 可高效访问的内存中。
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
        indices.data(),
        GL_STATIC_DRAW
    );

    /*
        当前 OpenGL Context 概念状态表：

        对象表:
            VAO ID m_vao -> VAO 对象
                location 配置：尚未设置
                element array buffer = m_ebo

            Buffer ID m_vbo -> Buffer 对象
                保存数据：
                    vertices 顶点数据

            Buffer ID m_ebo -> Buffer 对象
                保存数据：
                    indices 索引数据
                    0, 1, 3,
                    1, 2, 3

        当前绑定状态：
            current VAO = m_vao
            current GL_ARRAY_BUFFER = m_vbo
            current GL_ELEMENT_ARRAY_BUFFER = m_ebo
                注意：该 EBO 绑定关系已记录在 m_vao 中
            current shader program = 不受本函数影响
    */


    // 配置 location = 0 的顶点属性读取规则。
    //
    // 对应顶点着色器中的：
    //
    //     layout(location = 0) in vec3 aPos;
    //
    // 参数说明：
    //
    //     0:
    //         配置 location = 0 的顶点属性。
    //
    //     3:
    //         每个顶点属性由 3 个分量组成，即 x、y、z。
    //
    //     GL_FLOAT:
    //         每个分量的数据类型是 float。
    //
    //     GL_FALSE:
    //         不进行归一化。
    //
    //     3 * sizeof(float):
    //         stride，相邻两个顶点之间的字节间隔。
    //
    //     nullptr:
    //         offset，从当前 VBO 的起始位置开始读取。
    //
    // 关键点：
    //
    //     当前 VAO = m_vao
    //     当前 GL_ARRAY_BUFFER = m_vbo
    //
    // 因此这句会把如下规则记录到 m_vao 中：
    //
    //     location 0:
    //         source buffer = m_vbo
    //         size = 3
    //         type = GL_FLOAT
    //         normalized = false
    //         stride = 3 * sizeof(float)
    //         offset = 0
    //
    // CPU:
    //     提供顶点属性配置参数。
    //
    // OpenGL Driver / Context:
    //     把 location = 0 的读取规则记录到当前 VAO m_vao。
    //
    // GPU:
    //     此时还不会读取顶点。
    //     真正读取发生在 glDrawElements。
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        5 * static_cast<GLsizei>(sizeof(float)),
        nullptr
    );

    /*
        当前 OpenGL Context 概念状态表：

        对象表:
            VAO ID m_vao -> VAO 对象
                location 0:
                    enabled = false
                    source buffer = m_vbo
                    size = 3
                    type = GL_FLOAT
                    normalized = false
                    stride = 3 * sizeof(float)
                    offset = 0

                element array buffer = m_ebo

            Buffer ID m_vbo -> Buffer 对象
                保存数据：
                    vertices 顶点数据

            Buffer ID m_ebo -> Buffer 对象
                保存数据：
                    indices 索引数据

        当前绑定状态：
            current VAO = m_vao
            current GL_ARRAY_BUFFER = m_vbo
            current GL_ELEMENT_ARRAY_BUFFER = m_ebo
            current shader program = 不受本函数影响
    */


    // 启用 location = 0 的顶点属性数组。
    //
    // 如果没有这句，即使前面配置了 glVertexAttribPointer，
    // 顶点着色器中的 aPos 也不会正常按数组方式从 VBO 中读取。
    //
    // CPU:
    //     调用 OpenGL API。
    //
    // OpenGL Driver / Context:
    //     修改当前 VAO m_vao 的状态：
    //         location 0 enabled = true
    //
    // GPU:
    //     暂时没有绘制动作。
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        5 * static_cast<GLsizei>(sizeof(float)),
        reinterpret_cast<void*>(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    /*
        当前 OpenGL Context 概念状态表：

        对象表:
            VAO ID m_vao -> VAO 对象
                location 0:
                    enabled = true
                    source buffer = m_vbo
                    size = 3
                    type = GL_FLOAT
                    normalized = false
                    stride = 3 * sizeof(float)
                    offset = 0

                element array buffer = m_ebo

            Buffer ID m_vbo -> Buffer 对象
                保存数据：
                    vertices 顶点数据

            Buffer ID m_ebo -> Buffer 对象
                保存数据：
                    indices 索引数据

        当前绑定状态：
            current VAO = m_vao
            current GL_ARRAY_BUFFER = m_vbo
            current GL_ELEMENT_ARRAY_BUFFER = m_ebo
            current shader program = 不受本函数影响
    */


    // 解绑 GL_ARRAY_BUFFER。
    //
    // 这句只是把当前 GL_ARRAY_BUFFER 绑定点清空：
    //
    //     current GL_ARRAY_BUFFER = 0
    //
    // 它不会删除 m_vbo。
    //
    // 为什么可以解绑？
    //
    //     因为 glVertexAttribPointer 已经把 location = 0 的 source buffer = m_vbo
    //     记录到 m_vao 里面了。
    //
    // 所以后续绘制时，只要绑定 m_vao，OpenGL 仍然知道 location 0 应该从 m_vbo 读取。
    //
    // CPU:
    //     调用 OpenGL API。
    //
    // OpenGL Driver / Context:
    //     修改当前绑定状态：
    //         current GL_ARRAY_BUFFER = 0
    //
    // GPU:
    //     没有绘制动作。
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /*
        当前 OpenGL Context 概念状态表：

        对象表:
            VAO ID m_vao -> VAO 对象
                location 0:
                    enabled = true
                    source buffer = m_vbo
                    size = 3
                    type = GL_FLOAT
                    normalized = false
                    stride = 3 * sizeof(float)
                    offset = 0

                element array buffer = m_ebo

            Buffer ID m_vbo -> Buffer 对象
                保存数据：
                    vertices 顶点数据

            Buffer ID m_ebo -> Buffer 对象
                保存数据：
                    indices 索引数据

        当前绑定状态：
            current VAO = m_vao
            current GL_ARRAY_BUFFER = 0
            current GL_ELEMENT_ARRAY_BUFFER = m_ebo
                注意：该 EBO 绑定关系仍然属于当前 VAO m_vao
            current shader program = 不受本函数影响
    */


    // 解绑 VAO。
    //
    // 这句只是把当前 VAO 绑定点清空：
    //
    //     current VAO = 0
    //
    // 它不会删除 m_vao。
    //
    // m_vao 中已经记录好的内容仍然保留：
    //
    //     location 0 从 m_vbo 读取
    //     element array buffer = m_ebo
    //
    // 注意：
    //
    //     不要在 glBindVertexArray(0) 之前调用：
    //
    //         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //
    //     因为 GL_ELEMENT_ARRAY_BUFFER 是 VAO 状态的一部分。
    //     如果在 VAO 仍绑定时解绑 EBO，会把 m_vao 记录的 EBO 清掉。
    //
    // CPU:
    //     调用 OpenGL API。
    //
    // OpenGL Driver / Context:
    //     修改当前绑定状态：
    //         current VAO = 0
    //
    // GPU:
    //     没有绘制动作。
    glBindVertexArray(0);

    /*
        当前 OpenGL Context 概念状态表：

        对象表:
            VAO ID m_vao -> VAO 对象
                location 0:
                    enabled = true
                    source buffer = m_vbo
                    size = 3
                    type = GL_FLOAT
                    normalized = false
                    stride = 3 * sizeof(float)
                    offset = 0

                element array buffer = m_ebo

            Buffer ID m_vbo -> Buffer 对象
                保存数据：
                    vertices 顶点数据

            Buffer ID m_ebo -> Buffer 对象
                保存数据：
                    indices 索引数据

        当前绑定状态：
            current VAO = 0
            current GL_ARRAY_BUFFER = 0
            current GL_ELEMENT_ARRAY_BUFFER = 取决于当前 VAO
                当前 VAO = 0，所以此处不再使用 m_vao 的 EBO 绑定状态
            current shader program = 不受本函数影响

        此时 initializeGeometry() 完成。

        后续 paintGL() 中执行：

            glBindVertexArray(m_vao);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        会恢复 m_vao 中记录的：

            location 0 -> source buffer = m_vbo
            element array buffer = m_ebo

        然后 GPU 才会根据 EBO 索引从 VBO 中读取顶点并执行绘制。
    */

}

void DisplayOpenGLImage::initializeTexture()
{
    // 注意：
    // 这里不使用 QImage / QOpenGLTexture 解码和创建纹理。
    // stb_image 直接读取图片，返回 CPU 内存中的像素数据，再交给原生 OpenGL Texture。
    //
    // 本函数必须在有效 OpenGL Context 中调用。
    // 在当前工程中，它由 initializeGL() 调用，Qt 会在调用 initializeGL() 前
    // 自动让当前 QOpenGLWidget 的 OpenGL Context 成为当前线程的 current context。
    //
    // 当前 OpenGL Context 概念状态表：
    //
    // 对象表：
    //     m_texture 可能为 0，也可能是之前遗留的 Texture Object ID。
    //
    // 当前绑定状态：
    //     active texture unit = 之前遗留状态
    //     当前纹理单元的 GL_TEXTURE_2D = 之前遗留状态
    //     current VAO = 不受本函数影响
    //     current shader program = 不受本函数影响

    const std::string texturePath = findTexturePath();
    if (texturePath.empty()) {
        std::cerr
            << "Texture file not found. Please save the image as: "
            << "assets/textures/ui/display_image.png"
            << std::endl;
        return;
    }

    // OpenGL 的纹理坐标通常认为 (0, 0) 是左下角；
    // 普通图片文件通常是左上角开始存储。
    // 设置 true 后，stb 加载时会把图片上下翻转，方便和 OpenGL 纹理坐标对应。
    //
    // 注意：
    //     这是 stb_image 的图片加载策略，不是 OpenGL Context 状态。
    //     它只影响后续 stbi_load() 返回的 CPU 像素数据排列方式。
    stbi_set_flip_vertically_on_load(true);

    int width = 0;
    int height = 0;
    int channels = 0;

    // 强制加载为 4 通道 RGBA。
    // 这样无论原图是 RGB / RGBA，都统一按照 GL_RGBA 上传。
    unsigned char* data = stbi_load(
        texturePath.c_str(),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha
    );

    /*
        当前阶段：

        CPU:
            如果 data != nullptr：
                data 指向 stb_image 分配的 CPU 像素数组。
                width / height 保存图片宽高。
                channels 保存原始图片通道数。
                因为指定了 STBI_rgb_alpha，data 实际按 RGBA 4 通道排列。

        OpenGL Context:
            还没有创建或修改 Texture Object。
            还没有上传像素到 GPU。

        GPU:
            暂时没有这张图片的纹理数据。
    */

    if (data == nullptr) {
        std::cerr << "Failed to load texture: "
                  << texturePath
                  << "\nReason: "
                  << stbi_failure_reason()
                  << std::endl;
        return;
    }

    if (width <= 0 || height <= 0) {
        std::cerr << "Texture image has invalid size: "
                  << texturePath
                  << std::endl;
        stbi_image_free(data);
        return;
    }

    // 如果 m_texture 之前已经持有一个 OpenGL Texture Object，
    // 先删除旧对象，避免泄漏 GPU/驱动资源。
    //
    // CPU:
    //     m_texture 是一个 GLuint ID。
    //
    // OpenGL Context:
    //     如果 m_texture != 0，glDeleteTextures 会把该 ID 对应的 Texture Object 标记为删除。
    //     若它当前仍绑定在某个纹理单元上，OpenGL 会根据规范处理其生命周期；
    //     本函数随后会重新创建并绑定新的纹理对象。
    if (m_texture != 0) {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }

    /*
        当前 OpenGL Context 概念状态表：

        对象表：
            旧的 Texture Object 已不再由 m_texture 持有。
            m_texture = 0。

        当前绑定状态：
            active texture unit = 之前遗留状态
            当前纹理单元的 GL_TEXTURE_2D = 之前遗留状态
            current VAO = 不受本函数影响
            current shader program = 不受本函数影响
    */

    // 创建1个纹理对象，并将纹理ID返回给m_texture
    //
    // glGenTextures 只生成纹理对象名称/ID。
    // 它还没有指定纹理类型，也还没有上传图片像素。
    glGenTextures(1, &m_texture);
    if (m_texture == 0) {
        std::cerr << "Failed to create OpenGL texture object." << std::endl;
        stbi_image_free(data);
        return;
    }

    /*
        当前 OpenGL Context 概念状态表：

        对象表：
            Texture ID m_texture -> Texture Object
                target 类型：尚未由本函数绑定确定
                图像存储：尚未分配
                wrap / filter 参数：尚未由本函数设置

        当前绑定状态：
            active texture unit = 之前遗留状态
            当前纹理单元的 GL_TEXTURE_2D = 之前遗留状态
            current VAO = 不受本函数影响
            current shader program = 不受本函数影响
    */

    // 将这个纹理对象绑定到OpenGL Context的当前绑定状态中的GL_Texture
    //
    // 注意：
    //     glBindTexture 操作的是“当前 active texture unit”。
    //     如果之前没有显式调用 glActiveTexture，默认通常是 GL_TEXTURE0。
    //
    // 从这句开始，后续 glTexParameteri / glTexImage2D 都会作用于 m_texture。
    glBindTexture(GL_TEXTURE_2D, m_texture);

    /*
        当前 OpenGL Context 概念状态表：

        对象表：
            Texture ID m_texture -> Texture Object
                target 类型：GL_TEXTURE_2D
                图像存储：尚未分配
                wrap / filter 参数：尚未由本函数设置

        当前绑定状态：
            active texture unit = 之前遗留状态，通常是 GL_TEXTURE0
            当前 active texture unit 的 GL_TEXTURE_2D = m_texture
            current VAO = 不受本函数影响
            current shader program = 不受本函数影响

        重要区别：
            纹理绑定不是 VAO 状态。
            后续绘制前仍需要在 paintGL() 中绑定纹理到纹理单元。
    */

    // 纹理环绕方式。
    // 超出 [0, 1] 的纹理坐标时，使用边缘颜色。
    // s轴是纹理坐标系的横坐标、t轴是纹理坐标系的纵坐标
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 纹理过滤方式。
    // 缩小时、放大时都使用线性过滤。
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /*
        当前 OpenGL Context 概念状态表：

        对象表：
            Texture ID m_texture -> Texture Object
                target 类型：GL_TEXTURE_2D
                图像存储：尚未分配
                wrap S = GL_CLAMP_TO_EDGE
                wrap T = GL_CLAMP_TO_EDGE
                min filter = GL_LINEAR
                mag filter = GL_LINEAR

        当前绑定状态：
            当前 active texture unit 的 GL_TEXTURE_2D = m_texture
    */

    // 按 1 字节对齐读取 CPU 图像数据。
    // 对 RGBA 来说通常不是必须，但保留这个设置更稳。
    //
    // GL_UNPACK_ALIGNMENT 是 OpenGL Context 的像素解包状态，
    // 不是 Texture Object 自己的属性。
    // 它会影响 glTexImage2D 从 CPU data 读取每一行像素时如何处理字节对齐。
    //
    // 这里先保存旧值，上传完成后再恢复，避免影响后续其他纹理上传。
    GLint previousUnpackAlignment = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /*
        当前 OpenGL Context 概念状态表：

        对象表：
            Texture ID m_texture -> Texture Object
                图像存储：尚未分配
                wrap / filter 参数：已设置

        当前绑定状态：
            当前 active texture unit 的 GL_TEXTURE_2D = m_texture
            GL_UNPACK_ALIGNMENT = 1
    */

    // 把 CPU 端图片数据上传到当前绑定的 OpenGL Texture Object。
    //
    // 因为前面已经：
    //     glBindTexture(GL_TEXTURE_2D, m_texture);
    //
    // 所以这里实际定义并上传的是 m_texture 的第 0 级二维纹理图像。
    //
    // 参数含义：
    //     GL_TEXTURE_2D:
    //         操作当前纹理单元的二维纹理绑定点。
    //
    //     0:
    //         Mipmap level 0，也就是原始最高分辨率图像。
    //
    //     GL_RGBA8:
    //         GPU 内部存储格式，表示每个像素按 8-bit RGBA 保存。
    //
    //     width / height:
    //         纹理图像尺寸。
    //
    //     GL_RGBA / GL_UNSIGNED_BYTE:
    //         CPU data 的像素排列格式和分量类型。
    //
    //     data:
    //         stb_image 返回的 CPU 像素数组。
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        data
    );

    /*
        当前 OpenGL Context 概念状态表：

        对象表：
            Texture ID m_texture -> Texture Object
                target 类型：GL_TEXTURE_2D
                level 0 图像存储：
                    internal format = GL_RGBA8
                    size = width x height
                    source format = GL_RGBA
                    source type = GL_UNSIGNED_BYTE
                wrap S = GL_CLAMP_TO_EDGE
                wrap T = GL_CLAMP_TO_EDGE
                min filter = GL_LINEAR
                mag filter = GL_LINEAR

        当前绑定状态：
            当前 active texture unit 的 GL_TEXTURE_2D = m_texture
            GL_UNPACK_ALIGNMENT = 1

        GPU / Driver:
            data 指向的 CPU 像素内容已经交给 OpenGL。
            驱动负责把它放到合适的 GPU 可访问存储中。
    */

    // 恢复进入本函数前的像素解包对齐状态。
    //
    // CPU:
    //     previousUnpackAlignment 保存旧状态值。
    //
    // OpenGL Context:
    //     GL_UNPACK_ALIGNMENT 恢复为旧值，避免污染后续 OpenGL 调用。
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);

    /*
        当前 OpenGL Context 概念状态表：

        对象表：
            Texture ID m_texture -> Texture Object
                level 0 图像数据已经存在
                wrap / filter 参数已经存在

        当前绑定状态：
            当前 active texture unit 的 GL_TEXTURE_2D = m_texture
            GL_UNPACK_ALIGNMENT = previousUnpackAlignment
    */

    // 解绑当前纹理单元上的 GL_TEXTURE_2D。
    //
    // 这只是清空当前绑定点：
    //
    //     当前 active texture unit 的 GL_TEXTURE_2D = 0
    //
    // 它不会删除 m_texture，也不会清除已经上传到 m_texture 的图像数据。
    glBindTexture(GL_TEXTURE_2D, 0);

    /*
        当前 OpenGL Context 概念状态表：

        对象表：
            Texture ID m_texture -> Texture Object
                level 0 图像数据已经存在
                wrap / filter 参数已经存在

        当前绑定状态：
            当前 active texture unit 的 GL_TEXTURE_2D = 0
            GL_UNPACK_ALIGNMENT = previousUnpackAlignment

        后续 paintGL() 绘制前会重新执行：
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_texture);

        这样片段着色器中的 sampler2D uTexture 才能通过纹理单元 0 找到 m_texture。
    */

    // 释放 stb_image 分配的 CPU 端图片内存。
    // 注意：这不会影响 OpenGL Texture，因为 glTexImage2D 已经把数据交给 OpenGL 了。
    stbi_image_free(data);

    /*
        initializeTexture() 完成后的结果：

        CPU：
            stb_image 分配的 data 已释放。
            m_texture 保存 OpenGL Texture Object ID。

        OpenGL Context 对象表：
            Texture ID m_texture -> Texture Object
                保存 display_image.png 上传后的 RGBA8 像素数据。
                保存 wrap / filter 参数。

        OpenGL Context 当前绑定状态：
            当前 active texture unit 的 GL_TEXTURE_2D = 0
            current VAO = 不受本函数影响
            current shader program = 不受本函数影响

        后续绘制关系：
            paintGL()
                -> glUseProgram(m_shaderProgram)
                -> glActiveTexture(GL_TEXTURE0)
                -> glBindTexture(GL_TEXTURE_2D, m_texture)
                -> glBindVertexArray(m_vao)
                -> glDrawElements(...)

            片段着色器：
                sampler2D uTexture = 0
                    -> 纹理单元 GL_TEXTURE0
                    -> GL_TEXTURE0 的 GL_TEXTURE_2D 绑定 m_texture
                    -> texture(uTexture, TexCoord) 从 m_texture 采样颜色
    */

    m_textureWidth = width;
    m_textureHeight = height;
    m_cameraTextureAllocated = false;
}

void DisplayOpenGLImage::uploadPendingCameraFrame()
{
    if (!m_hasPendingCameraFrame) {
        return;
    }

    if (m_pendingFrameWidth <= 0 || m_pendingFrameHeight <= 0 || m_pendingRgb24Frame.empty()) {
        m_hasPendingCameraFrame = false;
        return;
    }

    if (m_texture == 0) {
        glGenTextures(1, &m_texture);
        if (m_texture == 0) {
            std::cerr << "Failed to create camera texture object." << std::endl;
            return;
        }
    }

    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLint previousUnpackAlignment = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    const bool needAllocate =
        !m_cameraTextureAllocated
        || m_textureWidth != m_pendingFrameWidth
        || m_textureHeight != m_pendingFrameHeight;

    if (needAllocate) {
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGB8,
            m_pendingFrameWidth,
            m_pendingFrameHeight,
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            m_pendingRgb24Frame.data()
        );

        m_textureWidth = m_pendingFrameWidth;
        m_textureHeight = m_pendingFrameHeight;
        m_cameraTextureAllocated = true;
    }
    else {
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            m_pendingFrameWidth,
            m_pendingFrameHeight,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            m_pendingRgb24Frame.data()
        );
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_pendingRgb24Frame.clear();
    m_hasPendingCameraFrame = false;
}

void DisplayOpenGLImage::cleanup()
{
    if (m_texture != 0) {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }

    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }

    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }

    if (m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }

    if (m_shaderProgram != 0) {
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
    }
}

