# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required(VERSION 3.0.0)
project(DemoUI VERSION 0.1.0)
string(TIMESTAMP RUN_TIME)
message( " project start : " ${RUN_TIME})


set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES  ON)

set(CMAKE_BUILD_TYPE Release)


get_directory_property(_has_parent PARENT_DIRECTORY)
if(_has_parent)
  set(is_root_project OFF)
else()
  set(is_root_project ON)
endif()

set(NE_DEVELOPER_DEFAULTS "${is_root_project}" CACHE BOOL "Turns on default settings for development of QTdemo")

option(BUILD_EXAMPLES "Build Examples" "${NE_DEVELOPER_DEFAULTS}")
option(BUILD_MODULES "Build Modules" "${NE_DEVELOPER_DEFAULTS}")
option(BUILD_DEBUG_POSTFIX_D "Append d suffix to debug libraries" OFF)

# 默认生成参数设置
if(NE_DEVELOPER_DEFAULTS)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_EXTENSIONS OFF)
  # PROJECT_BINARY_DIR 指build生成所在文件夹
  # PROJECT_SOURCE_DIR 指最外层CMakeLists.txt所在文件夹
  # CMAKE_CURRENT_SOURCE_DIR 指正在处理的CMakeLists.txt所在文件夹
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
  SET(PROJECT_ROOT_PATH ${PROJECT_SOURCE_DIR})

  message("params" PROJECT_BINARY_DIR)
endif()

set(TS_FILES
    "${CMAKE_SOURCE_DIR}/translations/zh_CN.ts"
    "${CMAKE_SOURCE_DIR}/translations/en_US.ts"
)


find_program(LUPDATE_EXECUTABLE lupdate)
find_program(LRELEASE_EXECUTABLE lrelease)
foreach(_ts_file ${TS_FILES})
    execute_process(
        COMMAND ${LUPDATE_EXECUTABLE} -recursive ${CMAKE_SOURCE_DIR} -ts ${_ts_file})
    execute_process(
        COMMAND ${LRELEASE_EXECUTABLE} ${_ts_file})
endforeach()
##################
# 获取默认库路径
# 记录 // include // lib // 子文件夹名
##
if(ON)
   
    # SUBDIRENAMES 遍历子文件夹  
    # ISAbsolutePath ON 子文件夹路径名     OFF 读取文件名 
    macro(SUBDIRENAMES result curdir _temp ISAbsolutePath)
        file(GLOB children RELATIVE ${curdir} ${curdir}/*${_temp}) 
        set(dirlist "")
        foreach(child ${children})
            if(IS_DIRECTORY ${curdir}/${child} )
                if(${child}  MATCHES .*${_temp})
                    if(${ISAbsolutePath})
                        LIST(APPEND dirlist ${curdir}/${child})
                    else()
                        LIST(APPEND dirlist ${child})
                    endif()
                endif()
            endif() 
        endforeach()
        set(${result} ${dirlist})
    endmacro()
    ##########

    ####################### 自动查询路径下库文件 include  lib  拷贝dll
    # 
    macro(FINDDLLDIR result Includes Libs ISMoveDll)
        SET(envs "")
        if(${ISMoveDll}) 
            if(EXISTS ${result}/dll)
                message("[!] Move dll :" ${result}/dll )
                file(COPY ${result}/dll/ DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
            else()
                message("[!] Move dll Fail Dir is not exists !" ${result}/dll)
            endif()           
        endif()
        
        list(APPEND ${Includes} ${result}/include)
        SUBDIRENAMES(envs ${result}/include "" ON)
        list(APPEND ${Includes} ${envs})
        SUBDIRENAMES(envs ${result} "lib" ON)
        list(APPEND ${Libs} ${envs})
    endmacro()
    ######################### 去除重复项
    #
    macro(REMOVE_DUPLICATES result )
        SET(DUP "")
        message("[!] RESULT  : " ${result})
        foreach(var ${result})
            list(FIND DUP ${var} _index)
            if(${_index} GREATER -1)
                message("[!] HAVE : " ${var} )
            else()
                message("[!] ADD  : " ${var})
                list(APPEND DUP ${var})
            endif()           
        endforeach()
        set(${result} ${DUP})
    endmacro()
    #########################
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        if(${LinuxSystem} STREQUAL "ARM")
            message("当前未配置Linux路径!")
        endif()
    else()
        if(MSVC)
            message(STATUS "MSVC version: ${MSVC_VERSION}")
             #  遍历ENV 文件夹下库  
             #  当前已有库 opencv410 halcon18.05  handdleAI2.2.5.6  基恩士keyence3D 海康mvs3.3.1
            SET(ENVPATH ${PROJECT_ROOT_PATH}/env) #设置遍历库路径
            SUBDIRENAMES(envs ${ENVPATH} "" OFF) #自动遍历
            # 设置引用库名
            #set(envs "opencv" "halcon" "keyence3D" "keyence2_5D" "mvs" "handdleAI" "Dalsa" "LMI" "3dAI" "927Dll")#手动设置引用库模块
            foreach(child  ${envs})
                message("DLL NAME: " ${child})
                FINDDLLDIR(${ENVPATH}/${child} ${child}_include ${child}_lib ON)
                message(${child}_include: ${${child}_include} )
                message(${child}_lib: ${${child}_lib})
                if(${child} MATCHES "opencv")
                    if(CMAKE_BUILD_TYPE MATCHES "Debug")
                        SET( ${child}_target_link "opencv_world410d" )
                    else()
                        SET( ${child}_target_link "opencv_world410" )
                    endif()                  					
                elseif(${child} MATCHES "halcon")
                    SET( ${child}_target_link 
                        "halcon"
                        "halconcpp"
                        "hdevenginecpp"     )                
                elseif(${child} MATCHES "handdleAI")
                   if(CMAKE_BUILD_TYPE MATCHES "Debug")
                        SET( ${child}_target_link "handleAId" )
                    else()
                        SET( ${child}_target_link "handleAI" )
                    endif()     		
                elseif(${child} MATCHES "mvs")
                        SET( ${child}_target_link "MvCameraControl" )
                elseif(${child} MATCHES "keyence2_5D")
                    SET( ${child}_target_link  "kglib" )  
                elseif(${child} MATCHES "keyence3D")
                    SET( ${child}_target_link 
                        "LJX8_IF" )
                endif()
                message(${child}_target_link: ${${child}_target_link})
            endforeach()

            # QT path qt5.14.2
            
            set(Qt5_DIR ${PROJECT_ROOT_PATH}/env/qt5.15.2/lib/cmake/Qt5)
            set(QT_DIR ${Qt5_DIR})
            set(qtlist "Core" "Widgets" "Charts" "Network" "Gui" "Concurrent" "Sql" "PrintSupport")
            #find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
            message("QT_Version :" Qt${QT_VERSION_MAJOR})


            add_definitions(-DQT_MESSAGELOGCONTEXT)#打开QT日志信息
        endif()
    endif()  
endif()



##################
# 设置dll后缀
##
if(BUILD_DEBUG_POSTFIX_D)
  set(CMAKE_DEBUG_POSTFIX d)
endif()
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")

if(CMAKE_BUILD_TYPE MATCHES "Release")
    #release生成调试信息
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

##################
# Automated modules
if(BUILD_MODULES)
   add_subdirectory(models)
endif()
# 包含子项目。
##################
# Automated examples
##
if(BUILD_EXAMPLES)
  #add_subdirectory(CppCode)
endif()


#install(FILES python_code/script.py DESTINATION ${CMAKE_INSTALL_PREFIX}/python_code)
#add_subdirectory(pycode)
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

string(TIMESTAMP RUN_TIME)
message( " project end : "  ${RUN_TIME})