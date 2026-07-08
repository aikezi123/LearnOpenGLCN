#pragma once

#include <cstdint>
#include <vector>

namespace learnopengl::domain {

enum class PixelFormat {
    Rgb24
};

struct VideoFrame final {
    int width{0};
    int height{0};
    std::uint64_t frameId{0};
    PixelFormat pixelFormat{PixelFormat::Rgb24};
    std::vector<unsigned char> pixels;
};

} // namespace learnopengl::domain
