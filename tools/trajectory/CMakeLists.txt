cmake_minimum_required(VERSION 3.21)

project(TrajectoryTools LANGUAGES CXX)

add_executable(export_archimedean_spiral_txt
    ExportArchimedeanSpiralTxt.cpp
    ../../domain/trajectory/src/ArchimedeanSpiral2DGenerator.cpp
)

target_include_directories(export_archimedean_spiral_txt
    PRIVATE
        ../../domain/trajectory/include
)

target_compile_features(export_archimedean_spiral_txt
    PRIVATE
        cxx_std_17
)

if(MSVC)
    target_compile_options(export_archimedean_spiral_txt PRIVATE /utf-8 /EHsc)
endif()
