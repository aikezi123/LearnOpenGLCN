#include <DisplayOpenGLImage.h>

#include <array>
#include <iostream>

namespace {

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

DisplayOpenGLImage::~DisplayOpenGLImage() {
    makeCurrent();
    cleanup();
    doneCurrent();
}

void DisplayOpenGLImage::initializeGL()
{
    initializeOpenGLFunctions();

    initializeShader();
    initializeGeometry();

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
}

void DisplayOpenGLImage::resizeGL(int width, int height) {
    // 从当前framebuffer的左下角开始。使用宽度width、高度height的矩形区域作为OpenGL的绘制区域
    glViewport(0, 0, width, height);
}

void DisplayOpenGLImage::paintGL() {
    // 它清空当前 QOpenGLWidget 的颜色缓冲区framebuffer
    glClear(GL_COLOR_BUFFER_BIT);
    // 使用指定的 Shader Program
    glUseProgram(m_shaderProgram);
    // 把 m_vao 设置为当前 OpenGL Context 中的“当前 VAO”。
    glBindVertexArray(m_vao);
    // 根据当前绑定的 VAO 和 VAO 中记录的 EBO，用索引方式绘制图元。
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    // 解绑当前 VAO。9代表解绑，而不是绑定location 0。
    glBindVertexArray(0);
}

void DisplayOpenGLImage::initializeShader()
{
    // 顶点着色器
    const char *vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";

    // 片段着色器
    const char *fragShaderSource = R"(
        #version 330 core
        layout(location = 0) out vec4 fragColor;
        void main() {
            fragColor = vec4(1.0, 0.5, 0.2, 1.0);
        }
    )";

    // 编译顶点着色器
    unsigned int vertexShaderID = compileShader(*this, GL_VERTEX_SHADER, vertexShaderSource);
    // 编译片段着色器
    unsigned int fragShaderID = compileShader(*this, GL_FRAGMENT_SHADER, fragShaderSource);
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

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragShaderID);
}

void DisplayOpenGLImage::initializeGeometry()
{
    const std::array<float, 12> vertices{
         0.5F,  0.5F, 0.0F,
         0.5F, -0.5F, 0.0F,
        -0.5F, -0.5F, 0.0F,
        -0.5F,  0.5F, 0.0F
    };

    const std::array<unsigned int, 6> indices{
        0, 1, 3,
        1, 2, 3
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
        indices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * static_cast<GLsizei>(sizeof(float)),
        nullptr
    );
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void DisplayOpenGLImage::cleanup()
{
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

