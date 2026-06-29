#include "hello_triangle.h"
#include "hello_window.h"
#include "shader_exercise.h"
#include "start_coordinateTransformation.h"
#include "start_shaders.h"
#include "start_textrues.h"

#include <iostream>
#include <string_view>

namespace
{
    int runShaderExercise()
    {
        learnopengl::getting_started::runShaderExercise();
        return 0;
    }

    struct LessonEntry
    {
        std::string_view name;
        int (*run)();
        std::string_view description;
    };

    constexpr LessonEntry kLessons[] = {
        {"hello_window", hello_window, "Getting Started / Hello Window"},
        {"hello_triangle", hello_triangle, "Getting Started / Hello Triangle"},
        {"start_shaders", start_shaders, "Getting Started / Shaders"},
        {"shader_exercise", runShaderExercise, "Getting Started / Shader Exercise"},
        {"start_textures", start_textures, "Getting Started / Textures"},
        {"transform", transform, "Getting Started / Transformations"},
    };

    constexpr std::string_view kDefaultLesson = "start_textures";

    void printUsage(const char* executableName)
    {
        std::cout << "Usage:\n"
                  << "  " << executableName << " [lesson-name]\n\n"
                  << "Default lesson: " << kDefaultLesson << "\n\n"
                  << "Available lessons:\n";

        for (const LessonEntry& lesson : kLessons) {
            std::cout << "  " << lesson.name << " - " << lesson.description << '\n';
        }

        std::cout << std::flush;
    }
}

int main(int argc, char* argv[])
{
    const std::string_view requestedLesson = argc > 1 ? std::string_view(argv[1]) : kDefaultLesson;

    if (requestedLesson == "--help" || requestedLesson == "-h" || requestedLesson == "--list") {
        printUsage(argv[0]);
        return 0;
    }

    for (const LessonEntry& lesson : kLessons) {
        if (requestedLesson == lesson.name) {
            std::cout << "Running lesson: " << lesson.name << " - " << lesson.description << '\n';
            return lesson.run();
        }
    }

    std::cerr << "Unknown lesson: " << requestedLesson << "\n\n";
    printUsage(argv[0]);
    return 1;
}
