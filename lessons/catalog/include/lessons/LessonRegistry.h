#pragma once

#include <cstddef>
#include <string_view>

namespace engineeringlab::lessons {

using LessonRunFunction = int (*)();

struct LessonEntry {
    std::string_view id;
    std::string_view chapter;
    std::string_view title;
    std::string_view description;
    LessonRunFunction run;
};

const LessonEntry* lessonsBegin();
const LessonEntry* lessonsEnd();
std::size_t lessonCount();
const LessonEntry* findLesson(std::string_view id);
std::string_view defaultLessonId();

} // namespace engineeringlab::lessons
