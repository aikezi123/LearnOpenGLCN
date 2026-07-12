#pragma once

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>

#include <vector>

class QResizeEvent;

// 使用 OpenGL 显示静态图片或相机 RGB24 帧，并提供翻转、旋转、缩放、平移及形状裁剪功能。
// QOpenGLWidget 负责窗口和 OpenGL 上下文生命周期，QOpenGLFunctions_3_3_Core 提供 OpenGL 3.3 函数入口。
class DisplayOpenGLImage
    : public QOpenGLWidget
    , protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
    Q_PROPERTY(DisplayShape displayShape READ displayShape WRITE setDisplayShape)

public:
    // 控件最终呈现的外形；矩形为默认模式，圆形模式会同时使用正方形 viewport 和 QWidget mask。
    enum class DisplayShape
    {
        Rectangle, // 使用完整控件区域显示矩形图像。
        Circle     // 居中显示正方形图像区域，并把控件裁剪为圆形。
    };
    Q_ENUM(DisplayShape)

    // 创建显示控件；OpenGL 资源将在 Qt 随后调用 initializeGL() 时建立。
    explicit DisplayOpenGLImage(QWidget* parent = nullptr);
    // 在控件的 OpenGL 上下文有效时释放纹理、缓冲区和着色器程序。
    ~DisplayOpenGLImage() override;

    // 缓存一帧 RGB24 图像并请求重绘；真正的纹理上传在 paintGL() 的有效 OpenGL 上下文中完成。
    void setRgb24Frame(int width, int height, std::vector<unsigned char> pixels);

    // 设置是否沿水平方向镜像图像。
    void setFlipHorizontal(bool enabled);
    // 设置是否沿垂直方向镜像图像。
    void setFlipVertical(bool enabled);

    // 将当前图像顺时针旋转 90°。
    void rotateClockwise90();
    // 将当前图像逆时针旋转 90°。
    void rotateCounterClockwise90();
    // 设置缩放倍数；实现会把数值限制在允许的范围内。
    void setViewScale(float scale);
    // 设置图像在裁剪空间中的绝对平移量。
    void setViewTranslation(float x, float y);
    // 在当前平移量上叠加一段位移。
    void panView(float deltaX, float deltaY);

    // 恢复翻转、旋转、缩放和平移的默认值，但不改变控件显示形状。
    void resetViewTransform();

    // 切换矩形或圆形显示，并同步更新控件 mask。
    void setDisplayShape(DisplayShape shape);
    
    // 返回当前控件显示形状。
    DisplayShape displayShape() const;

protected:
    // 初始化 OpenGL 函数入口、着色器、几何数据和初始纹理；调用期间上下文由 Qt 保证有效。
    void initializeGL() override;
    // 记录新的帧缓冲尺寸，供 paintGL() 根据显示形状设置 viewport。
    void resizeGL(int width, int height) override;
    // 上传待处理相机帧，设置本帧变换和裁剪参数，然后绘制纹理矩形。
    void paintGL() override;
    // 控件尺寸变化后重新生成圆形模式所需的 QWidget mask。
    void resizeEvent(QResizeEvent* event) override;

private:
    // 编译并链接顶点/片段着色器，把可用的 Program ID 保存到 m_shaderProgram。
    void initializeShader();
    // 创建并配置承载纹理矩形的 VAO、VBO 和 EBO。
    void initializeGeometry();
    // 创建纹理对象，并尝试载入启动时使用的默认图片。
    void initializeTexture();
    // 在当前 OpenGL 上下文中把待处理 RGB24 帧写入 m_texture。
    void uploadPendingCameraFrame();
    // 根据翻转、旋转、缩放和平移状态计算矩阵并写入 uTransform。
    void applyViewTransform();
    // 矩形模式使用完整 viewport，圆形模式使用居中的正方形 viewport。
    void applyDisplayViewport();
    // 圆形模式从纹理中央截取最大正方形，并设置纹理缩放及偏移 uniform。
    void applyTextureCrop();
    // 根据显示形状设置或清除 QWidget 的窗口级裁剪区域。
    void updateDisplayMask();
    // 删除本控件创建的全部 OpenGL 对象；调用前必须保证本控件上下文为 current。
    void cleanup();

private:
    unsigned int m_vao = 0;           // 纹理矩形的顶点数组对象 ID，记录顶点属性和索引缓冲绑定。
    unsigned int m_vbo = 0;           // 保存顶点位置及纹理坐标的顶点缓冲对象 ID。
    unsigned int m_ebo = 0;           // 保存两个三角形绘制索引的元素缓冲对象 ID。
    unsigned int m_texture = 0;       // 当前显示的二维纹理对象 ID，可由默认图片或相机帧填充。
    unsigned int m_shaderProgram = 0; // 绘制纹理矩形所使用的已链接着色器程序 ID。

    std::vector<unsigned char> m_pendingRgb24Frame; // 等待下一次 paintGL() 上传的 RGB24 像素副本。

    int m_pendingFrameWidth = 0;                    // 待上传相机帧的宽度，单位为像素。
    int m_pendingFrameHeight = 0;                   // 待上传相机帧的高度，单位为像素。

    int m_textureWidth = 0;                         // 当前纹理内容的宽度，用于判断重分配和计算正方形裁剪。
    int m_textureHeight = 0;                        // 当前纹理内容的高度，用于判断重分配和计算正方形裁剪。

    int m_framebufferWidth = 0;                     // QOpenGLWidget 帧缓冲的当前宽度，用于设置 viewport。
    int m_framebufferHeight = 0;                    // QOpenGLWidget 帧缓冲的当前高度，用于设置 viewport。

    bool m_hasPendingCameraFrame = false;           // 是否存在尚未上传到 GPU 的相机帧。
    bool m_cameraTextureAllocated = false;          // 当前纹理存储是否已按相机帧尺寸分配，可直接局部更新。

    bool m_flipHorizontal = false;                  // 是否在水平方向镜像图像。
    bool m_flipVertical = false;                    // 是否在垂直方向镜像图像。

    float m_rotationDegrees = 0.0F;                 // 当前旋转角度，单位为度；正值表示逆时针方向。
    float m_viewScale = 1.0F;                       // 当前统一缩放倍数。
    float m_translateX = 0.0F;                      // 图像在裁剪空间 X 方向的平移量。
    float m_translateY = 0.0F;                      // 图像在裁剪空间 Y 方向的平移量。

    DisplayShape m_displayShape = DisplayShape::Rectangle; // 当前显示形状，默认保持原有矩形控件行为。
};
