get_filename_component(CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(TIMESTAMP RUN_TIME)
message( " model start : " ${CURRENT_FOLDER}:${RUN_TIME})

set(OUR_LIBS "")
SUBDIRENAMES(OUR_LIBS ${CMAKE_CURRENT_SOURCE_DIR} "" OFF)
message("OUR_LIBS :" ${OUR_LIBS})
# 遍历子库
foreach(dll ${OUR_LIBS})
    add_subdirectory(${dll})
endforeach(dll ${})


set(DLL_NAMES "2Ddll" "opencv"  "handdleAI")
foreach(child ${DLL_NAMES})         
    LIST(APPEND ENV_INCLUDE ${${child}_include})
    LIST(APPEND ENV_LIB ${${child}_lib})
    LIST(APPEND ENV_TARGET ${${child}_target_link})         
endforeach()
message("   ENV_INCLUDE : " ${ENV_INCLUDE} )
message("   ENV_LIB : " ${ENV_LIB} )
message("   ENV_TARGET : " ${ENV_TARGET} )
include_directories(
    ${ENV_INCLUDE}
    ${PROJECT_ROOT_PATH}/lib/${CMAKE_BUILD_TYPE}
)

link_directories (
    ${ENV_LIB}
)
# qt引用

set(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)


find_package(Qt5 COMPONENTS ${qtlist} REQUIRED )
foreach(item ${qtlist})
    list(APPEND lib_list Qt5::${item})
endforeach()


FILE(GLOB HEAD_FILES "./*.h" )
FILE(GLOB SRC_FILES "./*.cpp")
FILE(GLOB UI_FILES "./*.ui")

INCLUDE_DIRECTORIES(${CMAEK_CURRENT_BINARY_DIR})

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
if(ANDROID)
  add_library(${CURRENT_FOLDER}  SHARED
      ${HEAD_FILES}
      ${SRC_FILES}
      ${UI_FILES}
)
else()
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        add_executable( ${CURRENT_FOLDER}
          ${HEAD_FILES}
          ${SRC_FILES}
          ${UI_FILES})
    else()
        add_executable( ${CURRENT_FOLDER} 
              ${HEAD_FILES}
              ${SRC_FILES}
              ${UI_FILES})
    endif()
  
endif()

SET(ALL_DLL 
    ${ENV_TARGET} 
    ${lib_list}
    
)
message("all_lib :" ${ALL_DLL})

target_link_libraries(${CURRENT_FOLDER}  ${ALL_DLL} )

string(TIMESTAMP RUN_TIME)
message( " model end : " ${CURRENT_FOLDER}:${RUN_TIME})
