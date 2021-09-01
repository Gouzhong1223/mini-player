# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)

# Declares and names the project.

project("miniplayer")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

#添加头文件路径
include_directories(${CMAKE_SOURCE_DIR}/../ffmpeg/include)
#ffmpeg库路径
set(libDir "${CMAKE_SOURCE_DIR}/../ffmpeg/libs/armeabi-v7a")

#使用下面这种方式包含ffmpeg库，编译后的apk中会包含这些库
#add_library(avcodec SHARED IMPORTED)
#set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${libDir}/libavcodec.so)
#
#add_library(avformat SHARED IMPORTED)
#set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${libDir}/libavformat.so)
#
#add_library(avfilter SHARED IMPORTED)
#set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION ${libDir}/libavfilter.so)
#
#add_library(swresample SHARED IMPORTED)
#set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${libDir}/libswresample.so)
#
#add_library(swscale SHARED IMPORTED)
#set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${libDir}/libswscale.so)
#
#add_library(avutil SHARED IMPORTED)
#set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${libDir}/libavutil.so)

#使用这种方式在链接选项加入ffmpeg库路径，编译后的apk中不会包含这些库，这两种方式都可以编译过，但第二种在运行时会出错
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${libDir}")

#message("xxx CMAKE_CXX_FLAGS")

add_library( # Sets the name of the library.
             native-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             native-lib.cpp )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native-lib
                       android
                       avutil
                       swresample
                       swscale
                       avcodec
                       avformat
                       avfilter
#                       ijkffmpeg
                       mediandk
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )