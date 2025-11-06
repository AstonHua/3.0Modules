# 获取获取当前目录名
get_filename_component(CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(TIMESTAMP RUN_TIME)
message( " model start : " ${CURRENT_FOLDER}:${RUN_TIME})
# target_compile_definitions(${CURRENT_FOLDER} PRIVATE HD_AIDEMO_MODULE_LIBRARY)

set(DLL_NAMES "opencv" "qt5")
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
    )

string(TIMESTAMP RUN_TIME)
message( " model end : " ${CURRENT_FOLDER}:${RUN_TIME})
