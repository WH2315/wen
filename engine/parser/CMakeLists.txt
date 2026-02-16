function(ParseHeaders TARGET_NAME INPUT_DIR OUTPUT_DIR)
    add_custom_target(${TARGET_NAME}_AUTO_GENERATE)

    set(PYTHON_BIN python)

    set(ENV{${TARGET_NAME}_OUTPUT_DIR} ${CMAKE_CURRENT_LIST_DIR}/${OUTPUT_DIR})
    set(ENV{${TARGET_NAME}_INPUT_DIR} ${CMAKE_CURRENT_LIST_DIR}/${INPUT_DIR})

    add_custom_command(
        TARGET ${TARGET_NAME}_AUTO_GENERATE
        PRE_BUILD
        COMMAND ${PYTHON_BIN} ${ParserPath}/parser.py
        $ENV{${TARGET_NAME}_OUTPUT_DIR}
        $ENV{${TARGET_NAME}_INPUT_DIR}
    )
endfunction()

function(SolveDependency TARGET_NAME)
    add_dependencies(${TARGET_NAME} ${TARGET_NAME}_AUTO_GENERATE)
    add_dependencies(${TARGET_NAME}_AUTO_GENERATE runtime)
endfunction()