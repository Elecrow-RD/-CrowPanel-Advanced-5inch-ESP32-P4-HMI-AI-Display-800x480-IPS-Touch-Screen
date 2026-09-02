# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# ESP-IDF 5.5 Kconfig writes UTF-8 metadata; force Python to decode it correctly
# when the project path contains non-ASCII characters.
set(ENV{PYTHONUTF8} 1)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(EXTRA_COMPONENT_DIRS "peripheral" "components")

# Keep imported binary libraries in the ASCII build path on Windows. GNU ld
# otherwise rejects archives whose source path contains non-ASCII characters.
function(add_prebuilt_library target_name lib_path)
    cmake_parse_arguments(_ "" "" "REQUIRES;PRIV_REQUIRES" ${ARGN})
    get_filename_component(
        lib_path "${lib_path}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")

    if(CMAKE_HOST_WIN32)
        string(SHA1 _prebuilt_hash "${lib_path}")
        get_filename_component(_prebuilt_name "${lib_path}" NAME)
        set(_prebuilt_copy
            "${CMAKE_BINARY_DIR}/prebuilt/${_prebuilt_hash}/${_prebuilt_name}")
        configure_file("${lib_path}" "${_prebuilt_copy}" COPYONLY)
        set(lib_path "${_prebuilt_copy}")
    endif()

    add_library(${target_name} STATIC IMPORTED)
    set_property(TARGET ${target_name} PROPERTY IMPORTED_LOCATION "${lib_path}")

    foreach(req ${__REQUIRES})
        idf_component_get_property(req_lib "${req}" COMPONENT_LIB)
        set_property(TARGET ${target_name} APPEND PROPERTY LINK_LIBRARIES "${req_lib}")
        set_property(TARGET ${target_name} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${req_lib}")
    endforeach()

    foreach(req ${__PRIV_REQUIRES})
        idf_component_get_property(req_lib "${req}" COMPONENT_LIB)
        set_property(TARGET ${target_name} APPEND PROPERTY LINK_LIBRARIES "${req_lib}")
        set_property(TARGET ${target_name} APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES "$<LINK_ONLY:${req_lib}>")
    endforeach()
endfunction()

project(5inch_Lesson13)

# GNU ld on Windows cannot open the prebuilt H.264 archives through a path
# containing non-ASCII characters. Keep the component unchanged and redirect
# only those two linker inputs to copies in the ASCII build directory.
if(CMAKE_HOST_WIN32 AND TARGET __idf_espressif__esp_h264)
    set(_h264_source_dir
        "${CMAKE_CURRENT_LIST_DIR}/managed_components/espressif__esp_h264/sw/libs/${IDF_TARGET}")
    set(_h264_binary_dir "${CMAKE_BINARY_DIR}/esp_h264_libs")
    file(MAKE_DIRECTORY "${_h264_binary_dir}")

    foreach(_h264_archive libtinyh264.a libopenh264.a)
        configure_file(
            "${_h264_source_dir}/${_h264_archive}"
            "${_h264_binary_dir}/${_h264_archive}"
            COPYONLY
        )
    endforeach()

    get_target_property(
        _h264_link_libraries
        __idf_espressif__esp_h264
        INTERFACE_LINK_LIBRARIES
    )
    list(FILTER _h264_link_libraries EXCLUDE REGEX "lib(tiny|open)h264\\.a$")
    list(APPEND _h264_link_libraries
        "${_h264_binary_dir}/libtinyh264.a"
        "${_h264_binary_dir}/libopenh264.a"
    )
    set_target_properties(__idf_espressif__esp_h264 PROPERTIES
        INTERFACE_LINK_LIBRARIES "${_h264_link_libraries}"
    )
endif()
