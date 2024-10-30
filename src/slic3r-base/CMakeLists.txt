set(SLIC3R_BASE_FILES
		include/Slic3r/Assert.hpp
		include/Slic3r/Log.hpp
		src/Slic3r/Log.cpp
		include/Slic3r/PlatformInfo.hpp
		src/Slic3r/PlatformInfo.cpp
		include/Slic3r/TypeInfo.hpp
		src/Slic3r/TypeInfo.cpp
)

if (APPLE)
	set(SLIC3R_BASE_FILES
			${SLIC3R_BASE_FILES}
			src/Slic3r/PlatformInfo_osx.mm
	)
elseif (WIN32)
	set(SLIC3R_BASE_FILES
			${SLIC3R_BASE_FILES}
			src/Slic3r/PlatformInfo_win32.cpp
	)
elseif (EMSCRIPTEN)
	set(SLIC3R_BASE_FILES
			${SLIC3R_BASE_FILES}
			src/Slic3r/PlatformInfo_emscripten.cpp
	)
elseif (UNIX)
	set(SLIC3R_BASE_FILES
			${SLIC3R_BASE_FILES}
			src/Slic3r/PlatformInfo_unix.cpp
	)
endif ()


add_library(slic3r-base STATIC ${SLIC3R_BASE_FILES})
target_include_directories(slic3r-base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(slic3r-base PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(slic3r-base PUBLIC
		spdlog
		libslic3r # To get semver
)

if(NOT EMSCRIPTEN)
	find_package(libassert REQUIRED)
	target_link_libraries(slic3r-base PUBLIC libassert::assert)
endif ()


# Doxygen
add_target_to_doxygen(slic3r-base)

