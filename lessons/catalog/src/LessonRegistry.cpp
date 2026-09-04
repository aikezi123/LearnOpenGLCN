#include <lessons/LessonRegistry.h>

#include "hello_triangle.h"
#include "hello_window.h"
#include "shader_exercise.h"
#include "start_coordinateTransformation.h"
#include "start_shaders.h"
#include "start_textrues.h"

#include <algorithm>
#include <iterator>

namespace {

int runShaderExercise()
{
    engineeringlab::getting_started::runShaderExercise();
    return 0;
}

constexpr engineeringlab::lessons::LessonEntry kLessons[] = {
    {
        "hello_window",
        "入门",
        "创建窗口",
        "Getting Started / Hello Window",
        hello_window
    },
    {
        "hello_triangle",
        "入门",
        "你好，三角形",
        "Getting Started / Hello Triangle",
        hello_triangle
    },
    {
        "start_shaders",
        "入门",
        "着色器",
        "Getting Started / Shaders",
        start_shaders
    },
    {
        "shader_exercise",
        "入门",
        "着色器练习",
        "Getting Started / Shader Exercise",
        runShaderExercise
    },
    {
        "start_textures",
        "入门",
        "纹理",
        "Getting Started / Textures",
        start_textures
    },
    {
        "transform",
        "入门",
        "变换",
        "Getting Started / Transformations",
        transform
    }
};

} // namespace

namespace engineeringlab::lessons {

const LessonEntry* lessonsBegin()
{
    return std::begin(kLessons);
}

const LessonEntry* lessonsEnd()
{
    return std::end(kLessons);
}

std::size_t lessonCount()
{
    return std::size(kLessons);
}

const LessonEntry* findLesson(std::string_view id)
{
    const auto iter = std::find_if(
        lessonsBegin(),
        lessonsEnd(),
        [id](const LessonEntry& lesson) {
            return lesson.id == id;
        }
    );

    return iter == lessonsEnd() ? nullptr : iter;
}

std::string_view defaultLessonId()
{
    return "start_textures";
}

} // namespace engineeringlab::lessons
