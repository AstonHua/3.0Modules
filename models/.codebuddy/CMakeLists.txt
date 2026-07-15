# CMakeList.txt: 遍历文件夹 添加子项目

set(OUR_LIBS "")
SUBDIRENAMES(OUR_LIBS ${CMAKE_CURRENT_SOURCE_DIR} "" OFF)
message("OUR_LIBS :" ${OUR_LIBS})
