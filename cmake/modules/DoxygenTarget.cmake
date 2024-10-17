
function(_get_target_libraries_with_prefix target_name prefix output_var)
    get_target_property(linked_libraries ${target_name} INTERFACE_LINK_LIBRARIES)
    #get_all_linked_libraries(${target_name} linked_libraries)

    if(NOT linked_libraries)
        return()
    endif()

    set(matching_libraries "")

    foreach(lib ${linked_libraries})
        string(FIND "${lib}" "${prefix}" prefix_pos)
        if(prefix_pos EQUAL 0)
            list(APPEND matching_libraries "${lib}")
        endif()
    endforeach()
    set(${output_var} "${matching_libraries}" PARENT_SCOPE)
endfunction()



set(MODULE_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

function(_get_doxygen_output_dir target output_var)
    set(${output_var} "${MODULE_SOURCE_DIR}/../../doc/doxy/${target}" PARENT_SCOPE)
endfunction()

function(_get_doxygen_tag_file target output_var)
    set(${output_var} "${MODULE_SOURCE_DIR}/../../doc/doxy/${target}/tag" PARENT_SCOPE)
endfunction()


function(_md_fix_fenced_blocks file_list out_dir base_dir out_files_var)
    # Initialize the list of output files
    set(output_files "")

    foreach(file_path IN LISTS file_list)
        get_filename_component(abs_file_path "${file_path}"
                REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        # Compute the relative path of the current file with respect to the base_dir
        file(RELATIVE_PATH relative_path "${base_dir}" "${abs_file_path}")

        # Construct the full destination path based on out_dir and relative_path
        set(destination_path "${out_dir}/${relative_path}")

        # Ensure the destination directory exists
        get_filename_component(destination_dir "${destination_path}" DIRECTORY)
        file(MAKE_DIRECTORY ${destination_dir})

        # Read the entire file as a list of lines (preserving semicolons safely)
        file(STRINGS "${file_path}" file_lines)

        # Prepare the modified content
        set(modified_content "")

        # Track whether we are inside a mermaid block
        set(in_mermaid_block FALSE)
        set(current_indent "")

        foreach(line IN LISTS file_lines)
            # Extract leading whitespace from the current line
            if("${line}" MATCHES "^[ \t]")
                string(REGEX MATCH "^[ \t]*" current_indent ${line})
            else()
                # Reset the indent if it's not a line with leading whitespace
                set(current_indent "")
            endif()

            if("${line}" MATCHES "^[ \t]*```mermaid")
                # Enter a mermaid block, replace the opening marker
                set(modified_content "${modified_content}${current_indent}<pre class=\"mermaid\">\n")
                set(in_mermaid_block TRUE)
            elseif("${line}" MATCHES "^[ \t]*```" AND in_mermaid_block)
                # Close the mermaid block, replace the closing marker
                set(modified_content "${modified_content}${current_indent}</pre>\n")
                set(in_mermaid_block FALSE)
            elseif("${line}" MATCHES "^[ \t]*```[A-za-z0-9+-]+")
                set(modified_content "${modified_content}${current_indent}```\n")
            else()
                # Regular line, add to modified content (including semicolons safely)
                set(modified_content "${modified_content}${line}\n")
            endif()

        endforeach()

        # Write the modified content to the destination path
        file(WRITE ${destination_path} "${modified_content}")

        #message(STATUS "Content written to ${destination_path}:\n${modified_content}")

        # Append the destination path to the output files list
        list(APPEND output_files ${destination_path})
    endforeach()

    # Return the list of output files to the parent scope
    set(${out_files_var} "${output_files}" PARENT_SCOPE)
endfunction()


find_package(Doxygen QUIET)

function(add_doxygen_target lib_target)
    if (NOT DOXYGEN_FOUND)
        message(STATUS "No doxygen found, doxygen related targets wont be defined")
        return()
    endif ()
    _get_target_libraries_with_prefix(${lib_target} "slic3r-" slicer_libs)

    set(DOXYGEN_TAGFILES "")
    set(LIB_DOXY_TAG_DEPS "")
    set(LIB_DOXY_DEPS "")
    foreach (lib ${slicer_libs})
        _get_doxygen_tag_file(${lib} _lib_tag_file)
        _get_doxygen_output_dir(${lib} _lib_doxy_dir)
        set(DOXYGEN_TAGFILES "${DOXYGEN_TAGFILES} ${_lib_tag_file}=${_lib_doxy_dir}/html")
        list(APPEND LIB_DOXY_TAG_DEPS "${lib}-doxygen-tag")
        list(APPEND LIB_DOXY_TAG_DEPS "${lib}-doxygen")
    endforeach ()

    set(DOXYGEN_PROJECT_NAME "${lib_target}")
    _get_doxygen_output_dir(${lib_target} DOXYGEN_OUTPUT_DIR)
    _get_doxygen_tag_file(${lib_target} DOXYGEN_TAG_FILE)

    file(GLOB_RECURSE DOXYGEN_OTHER_MARKDOWN
            RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
            LIST_DIRECTORIES false
            "${CMAKE_CURRENT_SOURCE_DIR}/*.md"
    )

    _md_fix_fenced_blocks("${DOXYGEN_OTHER_MARKDOWN}" "${CMAKE_CURRENT_BINARY_DIR}/doxy" "${CMAKE_CURRENT_SOURCE_DIR}" processed_files)

    set(readme_md_path "${CMAKE_CURRENT_BINARY_DIR}/doxy/README.md")
    if ("${readme_md_path}" IN_LIST processed_files)
        list(REMOVE_ITEM processed_files "${readme_md_path}")
        SET(DOXYGEN_README_PATH "${readme_md_path}")
    endif ()

    string(REPLACE ";" " " DOXYGEN_OTHER_MARKDOWN "${processed_files}")

    set(DOXYGEN_HTML_HEADER "${MODULE_SOURCE_DIR}/Doxygen-header.html")

    configure_file("${MODULE_SOURCE_DIR}/Doxyfile.in" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile" @ONLY)
    configure_file("${MODULE_SOURCE_DIR}/Doxyfile.tag.in" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.tag" @ONLY)

    add_custom_target("${lib_target}-doxygen-tag"
            COMMAND doxygen Doxyfile.tag
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
    add_custom_target("${lib_target}-doxygen"
            COMMAND doxygen Doxyfile
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            DEPENDS ${LIB_DOXY_TAG_DEPS} ${LIB_DOXY_DEPS}
    )

    file(MAKE_DIRECTORY "${DOXYGEN_OUTPUT_DIR}")

    if(NOT DEFINED DOXYGEN_TARGETS)
        set(DOXYGEN_TARGETS "" CACHE INTERNAL "List of all Doxygen targets")
    endif()

    set(DOXYGEN_TARGETS "${DOXYGEN_TARGETS};${lib_target}-doxygen"  CACHE INTERNAL "List of all Doxygen targets")
endfunction()

function(add_all_doxygen_target target)
    separate_arguments(DOXYGEN_TARGETS)
    if (DOXYGEN_TARGETS)
        add_custom_target(${target} DEPENDS ${DOXYGEN_TARGETS})
    else ()
        message(WARNING "No doxygen targets found, skipping definition of top-level all-doxygens target")
    endif ()
endfunction()
