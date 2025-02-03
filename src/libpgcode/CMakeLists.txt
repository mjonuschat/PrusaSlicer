cmake_minimum_required(VERSION 3.13)
project(libpgcode)

add_library(libpgcode
    include/libpgcode/LineView.hpp
    include/libpgcode/PostProcessorConfig.hpp
    include/libpgcode/Processor.hpp
    include/libpgcode/ProcessorConfig.hpp
    include/libpgcode/ProcessorResult.hpp
    include/libpgcode/Types.hpp
    include/libpgcode/Utils.hpp

    src/LineView.cpp
    src/OptionsZCorrector.hpp
    src/OptionsZCorrector.cpp
    src/PostProcessorConfig.cpp
    src/Processor.cpp
    src/ProcessorConfig.cpp
    src/ProcessorImpl.hpp
    src/ProcessorImpl.cpp
    src/ProcessorResult.cpp
    src/TimeBlock.hpp
    src/TimeBlock.cpp
    src/TimeMachine.hpp
    src/TimeMachine.cpp
    src/TimeProcessor.hpp
    src/TimeProcessor.cpp
    src/Types.cpp
    src/UsedFilaments.hpp
    src/UsedFilaments.cpp
    src/Utils.cpp
)

target_include_directories(libpgcode PUBLIC include)
target_include_directories(libpgcode PRIVATE src)

if (NOT EMSCRIPTEN)
    target_link_libraries(libpgcode PUBLIC boost_headeronly)
endif ()
target_link_libraries(libpgcode PRIVATE libslic3r)
