# 获取获取当前目录名
get_filename_component(CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(TIMESTAMP RUN_TIME)
message( " model start : " ${CURRENT_FOLDER}:${RUN_TIME})

set(DLL_NAMES "dahua" "opencv")
foreach(child ${DLL_NAMES})         
    LIST(APPEND ENV_INCLUDE ${${child}_include})
    LIST(APPEND ENV_LIB ${${child}_lib})
    LIST(APPEND ENV_TARGET ${${child}_target_link})         
endforeach()
LIST(APPEND ENV_INCLUDE ${CMAKE_GLOBAL_INCLUDE_DIRECTORY})
message("   ENV_INCLUDE : " ${ENV_INCLUDE} )
message("   ENV_LIB : " ${ENV_LIB} )
message("   ENV_TARGET : " ${ENV_TARGET} )

include_directories(

    ${ENV_INCLUDE}
    )

link_directories (
    ${ENV_LIB}
    )


set(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

FILE(GLOB HEAD_FILES "./*.h" )
FILE(GLOB SRC_FILES "./*.cpp")
FILE(GLOB UI_FILES "./*.ui")
FILE(GLOB GSRC_FILES "${CMAKE_GLOBAL_INCLUDE_DIRECTORY}/*.cpp")
LIST(APPEND SRC_FILES ${GSRC_FILES})
FILE(GLOB GHEAD_FILES "${CMAKE_GLOBAL_INCLUDE_DIRECTORY}/*.h")
LIST(APPEND HEAD_FILES ${GHEAD_FILES})
#find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)

add_library(${CURRENT_FOLDER} SHARED
    ${HEAD_FILES}
    ${SRC_FILES}
    ${UI_FILES}
    )



set_target_properties(${CURRENT_FOLDER} PROPERTIES LINKER_LANGUAGE C
    RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    LIBRARY_OUTPUT_DIRECTORY  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
    )

    find_package(Qt5 COMPONENTS ${qtlist} REQUIRED )
foreach(item ${qtlist})
    list(APPEND lib_list Qt5::${item})
endforeach()
target_link_libraries(${CURRENT_FOLDER} PRIVATE
    ${ENV_TARGET}
    ${lib_list}
    )


# target_compile_definitions(${CURRENT_FOLDER} PRIVATE HD_AIDEMO_MODULE_LIBRARY)

string(TIMESTAMP RUN_TIME)
message( " model end : " ${CURRENT_FOLDER}:${RUN_TIME})
