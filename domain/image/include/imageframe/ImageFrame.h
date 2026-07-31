#pragma once

#include <cstdint>
#include <vector>

namespace learnopengl::domain {

// 描述图像像素在内存中的排列格式。
enum class PixelFormat {
    Rgb24 // 每个像素依次保存红、绿、蓝三个 8 位通道。
};

// 与相机 SDK、Qt 和 OpenGL 无关的一帧图像数据。
// pixels 独占并连续保存像素内存，ImageFrame 销毁时会自动释放该内存。
struct ImageFrame final {
    int width{0};                                      // 图像宽度，单位为像素。
    int height{0};                                     // 图像高度，单位为像素。
    PixelFormat pixelFormat{PixelFormat::Rgb24};       // 当前像素排列格式。
    std::uint64_t frameId{0};                          // 图像在连续帧序列中的编号。
    std::vector<unsigned char> pixels;                 // 由本对象拥有的连续像素数据。
};

} // namespace learnopengl::domain
