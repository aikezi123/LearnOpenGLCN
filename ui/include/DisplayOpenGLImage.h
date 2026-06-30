#pragma once

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>

#include <vector>


// 双重继承，QOpenGLFunctions_3_3_Core为openGL的函数加载库
class DisplayOpenGLImage
    : public QOpenGLWidget
    , protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    explicit DisplayOpenGLImage(QWidget* parent = nullptr);
    ~DisplayOpenGLImage() override;

    // 接收相机 RGB24 帧。该函数应在 UI 线程调用；真正的 OpenGL 上传发生在 paintGL()。
    void setRgb24Frame(int width, int height, std::vector<unsigned char> pixels);

protected:
    // —————— ——Qt的回调函数重写。调用时自动会将当前QOpenGLWidget的上下文绑定到当前线程的context槽中，不需要手动makeCurrent(); ——————
    // 重写初始化函数。重写后的功能包括:1.初始化OpenGL函数入口 2.创建Shader Program 3.创建VAO/VBO/EBO，上传顶点和索引数据 4.初始化OpenGL状态
    void initializeGL() override;          
    // 每当 OpenGLWidget 尺寸变化时，让 OpenGL 的绘制区域重新铺满整个控件。
    void resizeGL(int width, int height) override;
    // 绘制当前帧。
    void paintGL() override;

private:
    // 初始化着色器。创建、编译、链接 OpenGL 着色器程序，并把最终可用于绘制的 Program ID 保存到 m_shaderProgram。
    void initializeShader();
    // 初始化集合资源。准备顶点数据及索引数据，创建、配置VAO、VBO、EBO对象。
    void initializeGeometry();
    // 初始化纹理对象
    void initializeTexture();
    // 如果有新的相机帧，在当前 OpenGL context 中上传到 m_texture。
    void uploadPendingCameraFrame();
    // 清理对象
    void cleanup();

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_ebo = 0;
    unsigned int m_texture = 0;
    unsigned int m_shaderProgram = 0;

    std::vector<unsigned char> m_pendingRgb24Frame;
    int m_pendingFrameWidth = 0;
    int m_pendingFrameHeight = 0;
    int m_textureWidth = 0;
    int m_textureHeight = 0;
    bool m_hasPendingCameraFrame = false;
    bool m_cameraTextureAllocated = false;
};
