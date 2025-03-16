set(SLIC3R_BASE_FILES
		include/Slic3r/Assert.hpp
		include/Slic3r/Log.hpp
		src/Slic3r/Log.cpp
		include/Slic3r/PlatformInfo.hpp
		src/Slic3r/PlatformInfo.cpp
		include/Slic3r/TypeInfo.hpp
		src/Slic3r/TypeInfo.cpp
		include/Slic3r/IdGenerator.hpp
        include/Slic3r/Exception.hpp
        include/Slic3r/Math.hpp
        include/Slic3r/Semver.hpp
        src/Slic3r/Semver.cpp
        include/Slic3r/Platform.hpp
        src/Slic3r/Platform.cpp
        include/Slic3r/Utils.hpp
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
# wxString conversions only with explicit encoding stated for all slic3r-* targets
if ((NOT UNIX) OR APPLE)
	# The wxNO_IMPLICIT_WXSTRING_ENCODING cannot be used along with wxUSE_UTF8_LOCALE_ONLY which gets defined on Linux
	# as part of autoconf generated file (setup.h.in)
	target_compile_definitions(slic3r-base PUBLIC -DwxNO_IMPLICIT_WXSTRING_ENCODING=1)
endif ()
target_include_directories(slic3r-base PUBLIC include)
target_include_directories(slic3r-base PRIVATE src)
target_link_libraries(slic3r-base PUBLIC
    spdlog
    semver
    boost_headeronly
)

if(NOT EMSCRIPTEN)
	find_package(libassert REQUIRED)
	slic3r_remap_configs("libassert::assert;cpptrace::cpptrace" RelWithDebInfo Release)
	target_link_libraries(slic3r-base PUBLIC libassert::assert)
endif ()


# Doxygen
add_target_to_doxygen(slic3r-base)

