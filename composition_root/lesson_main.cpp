#include "LessonLauncherWindow.h"

#include <lessons/LessonRegistry.h>

#include <QApplication>

#include <iostream>
#include <string_view>

namespace {

void printUsage(const char* executableName)
{
    std::cout << "Usage:\n"
              << "  " << executableName << "                Open lesson navigator\n"
              << "  " << executableName << " [lesson-id]    Run a lesson directly\n\n"
              << "Default navigator lesson: "
              << engineeringlab::lessons::defaultLessonId()
              << "\n\n"
              << "Available lessons:\n";

    for (const auto* lesson = engineeringlab::lessons::lessonsBegin();
         lesson != engineeringlab::lessons::lessonsEnd();
         ++lesson) {
        std::cout << "  " << lesson->id << " - " << lesson->description << '\n';
    }

    std::cout << std::flush;
}

int runLessonById(std::string_view id)
{
    const engineeringlab::lessons::LessonEntry* lesson = engineeringlab::lessons::findLesson(id);
    if (lesson == nullptr) {
        return -1;
    }

    std::cout << "Running lesson: " << lesson->id << " - " << lesson->description << '\n';
    return lesson->run();
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc > 1) {
        const std::string_view requestedLesson = argv[1];

        if (requestedLesson == "--help" || requestedLesson == "-h" || requestedLesson == "--list") {
            printUsage(argv[0]);
            return 0;
        }

        const int result = runLessonById(requestedLesson);
        if (result != -1) {
            return result;
        }

        std::cerr << "Unknown lesson: " << requestedLesson << "\n\n";
        printUsage(argv[0]);
        return 1;
    }

    QApplication app(argc, argv);

    engineeringlab::app::LessonLauncherWindow window;
    window.show();

    return app.exec();
}
