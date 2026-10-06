cmake_minimum_required(VERSION 3.24)
project(kde-dictation VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GNUInstallDirs)
include(CTest)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Network Multimedia WebSockets DBus Test)
find_package(KF6GlobalAccel CONFIG REQUIRED)
find_package(KF6StatusNotifierItem CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

if(TARGET yaml-cpp::yaml-cpp)
    set(KDICT_YAML_TARGET yaml-cpp::yaml-cpp)
else()
    set(KDICT_YAML_TARGET yaml-cpp)
endif()

add_library(kde_dictation_core STATIC
    src/app/DictationState.cpp
    src/app/FallbackPolicy.cpp
    src/app/LiveTranscriptReconciler.cpp
    src/audio/WavEncoder.cpp
    src/config/Config.cpp
    src/stt/RealtimeEventParser.cpp
    src/util/UrlHelpers.cpp
)

target_include_directories(kde_dictation_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(kde_dictation_core
    PUBLIC
        Qt6::Core
        Qt6::Gui
        ${KDICT_YAML_TARGET}
)

target_compile_options(kde_dictation_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(kde-dictation
    src/main.cpp
    src/app/DictationController.cpp
    src/audio/AudioRecorder.cpp
    src/output/TextOutput.cpp
    src/shortcut/ShortcutController.cpp
    src/stt/BatchOpenAIProvider.cpp
    src/stt/RealtimeAsrProvider.cpp
    src/ui/TrayController.cpp
)

target_link_libraries(kde-dictation
    PRIVATE
        kde_dictation_core
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Multimedia
        Qt6::WebSockets
        Qt6::DBus
        KF6::GlobalAccel
        KF6::StatusNotifierItem
)

target_compile_options(kde-dictation PRIVATE -Wall -Wextra -Wpedantic)

target_compile_definitions(kde-dictation PRIVATE
    KDICT_VERSION="${PROJECT_VERSION}"
)

install(TARGETS kde-dictation RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/org.amvc.kdedictation.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES config.example.yaml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/kde-dictation)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
