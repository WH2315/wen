set(ENGINE_ROOT_DIR "${WEN_ROOT_DIR}/engine")

function(organize_files directory)
    set(base_dir "${CMAKE_CURRENT_SOURCE_DIR}/${directory}")
    file(GLOB_RECURSE files CONFIGURE_DEPENDS "${base_dir}/*")

    foreach(file ${files})
        file(RELATIVE_PATH path ${base_dir} ${file})
        get_filename_component(subdirectory ${path} DIRECTORY)

        if(subdirectory)
            set(group "${directory}/${subdirectory}")
        else()
            set(group "${directory}")
        endif()

        source_group(${group} FILES ${file})
    endforeach()
endfunction()

add_subdirectory(runtime)
add_subdirectory(editor)