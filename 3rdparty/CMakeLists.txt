
# for faiss
set(FAISS_ENABLE_GPU OFF)
set(FAISS_ENABLE_PYTHON OFF)
set(BUILD_TESTING OFF)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/faiss")
    message("find faiss")
else()
    message("decompression faiss")
    execute_process(
        COMMAND tar -xzvf faiss.tar.gz
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE TAR_RESULT
    )

    if (NOT TAR_RESULT EQUAL "0")
        message(FATAL_ERROR "tar faiss.tar.gz failed: ${TAR_RESULT}")
    endif()
endif()

add_subdirectory(faiss)
