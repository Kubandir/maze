# Helper function to add precompiled headers
function(add_precompiled_header target_name header_name)
    if(MSVC)
        # MSVC specific implementation
        set_target_properties(${target_name} PROPERTIES COMPILE_FLAGS "/Yu${header_name}")
        get_target_property(sources ${target_name} SOURCES)
        set_source_files_properties(${sources} PROPERTIES COMPILE_FLAGS "/Yu${header_name}")
    elseif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # GCC/Clang implementation
        get_target_property(source_files ${target_name} SOURCES)
        foreach(source_file ${source_files})
            if(${source_file} MATCHES "\\.cpp$")
                set_source_files_properties(${source_file} PROPERTIES
                    COMPILE_FLAGS "-include ${header_name}"
                )
            endif()
        endforeach()
    endif()
endfunction()
