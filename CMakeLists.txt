cmake_minimum_required(VERSION 3.1.0)

project(vmslideviewer)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

if (NOT MSVC)
  find_package(PkgConfig)
  pkg_check_modules(OPENSLIDE REQUIRED openslide) 
  set(CMAKE_CXX_FLAGS_DEBUG "-g -pg -O0 -Wall -Wextra")
  set(CMAKE_CXX_FLAGS_RELEASE "-g -O3 -Wall -Wextra")
endif()

find_package(JPEG)
find_package(TIFF)
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

set ( MAIN_SRC
  src/safebmp.cc 
  src/slidecache.cc 
  src/vmdrawingarea.cc 
  src/vmslidemain.cc 
  src/vmslidewindow.cc 
  src/vmthreads.cc 
  src/vmzoomarea.cc
  src/vmtilereader.cc
  src/vmrangetool.cc
)

set ( MAIN_HEADERS
  include/safebmp.h
  include/slidecache.h
  include/vmslideviewer.h
  include/vmtilereader.h
)

set ( MAIN_RESOURCES
  resources/vmslideviewer.qrc
)
 
include_directories( ${LIBJPEG_INCLUDE_DIRS} ${LIBTIFF_INCLUDE_DIRS} ${OPENSLIDE_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (WIN32 OR WIN64 OR ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  list(APPEND MAIN_RESOURCES resources/images/vmslideviewer.ico)
endif()

add_executable(vmslideviewer ${MAIN_SRC} ${MAIN_HEADERS} ${MAIN_RESOURCES})

target_link_libraries(vmslideviewer Qt5::Widgets ${LIBJPEG_LIBRARIES} ${LIBTIFF_LIBRARIES} ${OPENSLIDE_LIBRARIES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  install(FILES data/vmslideviewer.desktop DESTINATION /usr/share/applications)
  install(FILES resources/images/microscope64x64.png DESTINATION /usr/share/icons/hicolor/64x64/apps RENAME vmslideviewer.png)
  install(FILES resources/images/microscope128x128.png DESTINATION /usr/share/icons/hicolor/128x128/apps RENAME vmslideviewer.png)
  install(FILES resources/images/microscope256x256.png DESTINATION /usr/share/icons/hicolor/256x256/apps RENAME vmslideviewer.png)
endif()

