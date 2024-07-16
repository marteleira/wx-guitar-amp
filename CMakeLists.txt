cmake_minimum_required(VERSION 3.16)
project(WxGuitarAmp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

set(SOURCES
    src/main.cpp
    src/Persistence.cpp
    src/audio/AudioEngine.cpp
    src/dsp/DspProcessor.cpp
    src/dsp/JCM800Model.cpp
    src/dsp/FenderDeluxeModel.cpp
    src/dsp/OrangeOR120Model.cpp
    src/ui/MainFrame.cpp
    src/ui/KnobControl.cpp
    src/ui/SpectrumView.cpp
)

add_executable(WxGuitarAmp ${SOURCES})

target_include_directories(WxGuitarAmp PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(WxGuitarAmp
    ${wxWidgets_LIBRARIES}
    pthread dl m
)

target_compile_options(WxGuitarAmp PRIVATE -O2 -ffast-math)
