cmake_minimum_required(VERSION 3.16)
project(OCCT_Industrial VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt's automatic Meta-Object Compiler (MOC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Use Qt Widgets instead of QML/Quick
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL)
find_package(OpenCASCADE REQUIRED)

qt_add_executable(appOCCT
    main.cpp
    OcctWidget.h OcctWidget.cpp
    .gitignore
    MainWindow.h MainWindow.cpp

)

# Link Qt and OCCT libraries
target_link_libraries(appOCCT
    PRIVATE
    Qt6::Widgets
    Qt6::OpenGL
    TKernel TKMath TKG2d TKG3d TKGeomBase TKGeomAlgo TKBRep TKTopAlgo
    TKMesh TKService TKOpenGl TKV3d TKXSBase TKSTEP TKSTEPBase TKSTEPAttr
    TKIGES TKShHealing
)

target_include_directories(appOCCT PRIVATE ${OpenCASCADE_INCLUDE_DIR})