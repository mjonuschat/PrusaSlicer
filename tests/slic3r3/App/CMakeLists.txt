get_filename_component(_DIR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(_TEST_NAME slic3r3_${_DIR_NAME})
set(_TEST_TARGET ${_TEST_NAME}_tests)
add_executable(${_TEST_TARGET}
        Scene/SceneTests.cpp
        slic3r3_app_tests.cpp
)

#find_package(glfw3 3.3 REQUIRED)
#target_link_libraries(slic3r3_app_tests PUBLIC slic3r3_app test_common glfw3)

target_link_libraries(${_TEST_TARGET} test_common slic3r3_app)

if (MSVC)
    target_link_libraries(${_TEST_TARGET} Setupapi.lib)
endif ()
set_target_properties(${_TEST_TARGET} PROPERTIES FOLDER "tests/slic3r3")
#set_property(TARGET ${_TEST_TARGET} PROPERTY FOLDER "tests/slic3r3")

if (WIN32)
    prusaslicer_copy_dlls(slic3r3_app_tests)
endif()

# catch_discover_tests(${_TEST_TARGET}_tests TEST_PREFIX "${_TEST_NAME}: ")
set(_catch_args "exclude:[NotWorking];-s")
list(APPEND _catch_args "${CATCH_EXTRA_ARGS}")
add_test(slic3r3_app_tests slic3r3_app_tests ${_catch_args})

message("${_TEST_NAME}")

get_target_property(OUT ${_TEST_TARGET} LINK_LIBRARIES)
message(STATUS ${OUT})


