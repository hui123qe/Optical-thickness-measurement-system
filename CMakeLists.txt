cmake_minimum_required(VERSION 3.21)

project(OpticalThicknessMeasurementUi VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Sql)

qt_standard_project_setup()

set(CMAKE_AUTORCC ON)

if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The packaged AGTIO and CL-3000 SDKs support MSVC x64 only.")
endif()

set(AGTIO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third-party/agtio")
set(AGTIO_DEBUG_DIR "${AGTIO_ROOT}/libs_8.2.1_debug_x64")
set(AGTIO_RELEASE_DIR "${AGTIO_ROOT}/libs_8.2.1_release_x64")

add_library(AgtioMotion SHARED IMPORTED GLOBAL)
set_target_properties(AgtioMotion PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${AGTIO_RELEASE_DIR}"
    IMPORTED_CONFIGURATIONS "Debug;Release"
    IMPORTED_IMPLIB_DEBUG "${AGTIO_DEBUG_DIR}/AG_Motion.lib"
    IMPORTED_LOCATION_DEBUG "${AGTIO_DEBUG_DIR}/AG_Motion.dll"
    IMPORTED_IMPLIB_RELEASE "${AGTIO_RELEASE_DIR}/AG_Motion.lib"
    IMPORTED_LOCATION_RELEASE "${AGTIO_RELEASE_DIR}/AG_Motion.dll"
    MAP_IMPORTED_CONFIG_MINSIZEREL Release
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
)
add_library(Agtio::Motion ALIAS AgtioMotion)

set(CL3000_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third-party/cl3000")
add_library(KeyenceCl3000 SHARED IMPORTED GLOBAL)
set_target_properties(KeyenceCl3000 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CL3000_ROOT}/include"
    IMPORTED_IMPLIB "${CL3000_ROOT}/lib/msvc/x64/CL3_IF.lib"
    IMPORTED_LOCATION "${CL3000_ROOT}/bin/msvc/x64/CL3_IF.dll"
)
add_library(Keyence::CL3000 ALIAS KeyenceCl3000)

qt_add_executable(OpticalThicknessUi
    src/main.cpp
    src/application_logger.cpp
    src/application_logger.h
    src/main_window.cpp
    src/main_window.h
    src/config/task_configuration.cpp
    src/config/task_configuration.h
    src/config/task_configuration_store.cpp
    src/config/task_configuration_store.h
    src/config/coordinate_transform_store.cpp
    src/config/coordinate_transform_store.h
    src/workflow/task_point_queue.cpp
    src/workflow/task_point_queue.h
    src/workflow/task_executor.cpp
    src/workflow/task_executor.h
    src/workflow/measurement_task_controller.cpp
    src/workflow/measurement_task_controller.h
    src/data/measurement_database.cpp
    src/data/measurement_database.h
    src/motor/motion_driver.h
    src/motor/agtio_motion_driver.cpp
    src/motor/agtio_motion_driver.h
    src/motor/virtual_motion_driver.cpp
    src/motor/virtual_motion_driver.h
    src/motor/motion_driver_factory.cpp
    src/motor/motion_driver_factory.h
    src/motor/axis_manager.cpp
    src/motor/axis_manager.h
    src/device/laser_probe.h
    src/device/cl3000_laser_driver.cpp
    src/device/cl3000_laser_driver.h
    src/device/virtual_laser_probe.cpp
    src/device/virtual_laser_probe.h
    src/device/device_manager.cpp
    src/device/device_manager.h
    src/view/automatic_operation_widget.cpp
    src/view/automatic_operation_widget.h
    src/view/status_widgets.cpp
    src/view/status_widgets.h
    src/view/operation_pages.cpp
    src/view/operation_pages.h
    src/view/debug_page.cpp
    src/view/debug_page.h
    src/view/log_page.cpp
    src/view/log_page.h
    src/view/widget_factory.cpp
    src/view/widget_factory.h
    src/view/application_style.cpp
    src/view/application_style.h
    resources/ui_resources.qrc
)

target_link_libraries(OpticalThicknessUi PRIVATE Qt6::Widgets Qt6::Sql Agtio::Motion Keyence::CL3000)

if(MSVC)
    target_compile_options(OpticalThicknessUi PRIVATE /utf-8)
endif()

set_target_properties(OpticalThicknessUi PROPERTIES
    WIN32_EXECUTABLE TRUE
)

set(AGTIO_RUNTIME_FILES
    AAComm.dll
    AACommServer.exe
    AACommServerAPI.dll
    AAMotion.dll
    AG_Motion.dll
    AKBMotion.dll
    YamlDotNet.dll
)
set(AGTIO_RUNTIME_DIR "$<IF:$<CONFIG:Debug>,${AGTIO_DEBUG_DIR},${AGTIO_RELEASE_DIR}>")
foreach(AGTIO_RUNTIME_FILE IN LISTS AGTIO_RUNTIME_FILES)
    add_custom_command(TARGET OpticalThicknessUi POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${AGTIO_RUNTIME_DIR}/${AGTIO_RUNTIME_FILE}"
            "$<TARGET_FILE_DIR:OpticalThicknessUi>/${AGTIO_RUNTIME_FILE}"
        VERBATIM
    )
endforeach()

add_custom_command(TARGET OpticalThicknessUi POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CL3000_ROOT}/bin/msvc/x64/CL3_IF.dll"
        "$<TARGET_FILE_DIR:OpticalThicknessUi>/CL3_IF.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:Qt6::Sql>"
        "$<TARGET_FILE_DIR:OpticalThicknessUi>/$<TARGET_FILE_NAME:Qt6::Sql>"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:OpticalThicknessUi>/sqldrivers"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:Qt6::QSQLiteDriverPlugin>"
        "$<TARGET_FILE_DIR:OpticalThicknessUi>/sqldrivers/$<TARGET_FILE_NAME:Qt6::QSQLiteDriverPlugin>"
    VERBATIM
)

add_custom_command(TARGET OpticalThicknessUi POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        "-DSOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/config/coordinate-transforms.json"
        "-DDESTINATION_FILE=$<TARGET_FILE_DIR:OpticalThicknessUi>/coordinate-transforms.json"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_file_if_missing.cmake"
    COMMAND ${CMAKE_COMMAND}
        "-DSOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/config/task-configurations.json"
        "-DDESTINATION_FILE=$<TARGET_FILE_DIR:OpticalThicknessUi>/task-configurations-v4.json"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_file_if_missing.cmake"
    COMMAND ${CMAKE_COMMAND}
        "-DSOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/config/data/virtual-outline.csv"
        "-DDESTINATION_FILE=$<TARGET_FILE_DIR:OpticalThicknessUi>/data/virtual-outline.csv"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_file_if_missing.cmake"
    COMMAND ${CMAKE_COMMAND}
        "-DSOURCE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/config/device-connections.json"
        "-DDESTINATION_FILE=$<TARGET_FILE_DIR:OpticalThicknessUi>/device-connections.json"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_file_if_missing.cmake"
    VERBATIM
)
