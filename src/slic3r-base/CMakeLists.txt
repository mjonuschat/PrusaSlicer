set(SLIC3R_BASE_FILES
	include/Slic3r/Log.hpp
	src/Slic3r/Log.cpp
)
add_library(slic3r-base STATIC ${SLIC3R_BASE_FILES})
target_include_directories(slic3r-base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(slic3r-base PUBLIC spdlog)


