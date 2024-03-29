CMAKE_MINIMUM_REQUIRED(VERSION 2.8...3.20)
PROJECT(TDengine)

INCLUDE_DIRECTORIES(${TD_COMMUNITY_DIR}/src/tsdb/inc)
INCLUDE_DIRECTORIES(${TD_COMMUNITY_DIR}/src/client/inc)
INCLUDE_DIRECTORIES(inc)

AUX_SOURCE_DIRECTORY(src SRC)
ADD_LIBRARY(query ${SRC})
SET_SOURCE_FILES_PROPERTIES(src/sql.c PROPERTIES COMPILE_FLAGS -w)
TARGET_LINK_LIBRARIES(query tsdb tutil lua)

IF (TD_LINUX)
  TARGET_LINK_LIBRARIES(query m rt lua)
  IF (NOT TD_NINGSI_60)
    ADD_SUBDIRECTORY(tests)
  ENDIF ()
ENDIF ()

IF (TD_DARWIN)
  TARGET_LINK_LIBRARIES(query m lua)
  IF (NOT TD_NINGSI_60)
    ADD_SUBDIRECTORY(tests)
  ENDIF ()
ENDIF ()
