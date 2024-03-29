CMAKE_MINIMUM_REQUIRED(VERSION 2.8...3.20)
PROJECT(TDengine)

INCLUDE_DIRECTORIES(${TD_COMMUNITY_DIR}/src/rpc/inc)
INCLUDE_DIRECTORIES(${TD_COMMUNITY_DIR}/src/sync/inc)
INCLUDE_DIRECTORIES(${TD_COMMUNITY_DIR}/deps/rmonotonic/inc)
INCLUDE_DIRECTORIES(${TD_COMMUNITY_DIR}/deps/TSZ/sz/include)

AUX_SOURCE_DIRECTORY(src SRC)
ADD_LIBRARY(tutil ${SRC})

TARGET_LINK_LIBRARIES(tutil pthread os lz4 z rmonotonic ${VAR_TSZ} )



IF (TD_LINUX)
  TARGET_LINK_LIBRARIES(tutil m rt)
  IF (NOT TD_NINGSI_60)
    ADD_SUBDIRECTORY(tests)
  ENDIF ()

  FIND_PATH(ICONV_INCLUDE_EXIST iconv.h /usr/include/ /usr/local/include/)
  IF (ICONV_INCLUDE_EXIST)
    FIND_PATH(ICONV_LIBRARY_A_EXIST libiconv.a /usr/lib/ /usr/local/lib/ /lib64)
    FIND_PATH(ICONV_LIBRARY_SO_EXIST libiconv.so /usr/lib/ /usr/local/lib/ /lib64)
    IF (ICONV_LIBRARY_A_EXIST OR ICONV_LIBRARY_SO_EXIST)
      MESSAGE(STATUS "Use the installed libiconv library")
      TARGET_LINK_LIBRARIES(tutil iconv)
    ELSE ()
      # libiconv library is already included in GLIBC,
      MESSAGE(STATUS "Use the iconv functions in GLIBC")
    ENDIF ()
  ELSE ()
    MESSAGE(STATUS "Failed to find iconv, use default encoding method")
  ENDIF ()

ELSEIF (TD_WINDOWS)
  TARGET_LINK_LIBRARIES(tutil iconv regex winmm IPHLPAPI ws2_32 wepoll)
ELSEIF(TD_DARWIN)
  TARGET_LINK_LIBRARIES(tutil m)
  TARGET_LINK_LIBRARIES(tutil iconv)
ENDIF()

IF (TD_STORAGE)
  TARGET_LINK_LIBRARIES(tutil storage)
ENDIF ()
