ADD_SUBDIRECTORY(lz4)

INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(boost_1_58/boost)
INCLUDE_DIRECTORIES(sdk)
INCLUDE_DIRECTORIES(pugixml/src)
INCLUDE_DIRECTORIES(dirty_searcher)

FILE(GLOB SDK_SRC sdk/*.h sdk/*.cpp)
FILE(GLOB PUGIXML pugixml/src/*.hpp pugixml/src/*.cpp)
FILE(GLOB NET sdk/net/*.h sdk/net/*.cpp)
FILE(GLOB HTTP sdk/net/http/*.hpp sdk/net/http/*.cpp)
FILE(GLOB DIRTY_SEARCHER dirty_searcher/*.h dirty_searcher/*.cpp)

SOURCE_GROUP(""             FILES ${SDK_SRC})
SOURCE_GROUP("pugixml"      FILES ${PUGIXML})
SOURCE_GROUP("net"          FILES ${NET})
SOURCE_GROUP("net\\http"     FILES ${HTTP})
SOURCE_GROUP("dirty_searcher" FILES ${DIRTY_SEARCHER})

ADD_LIBRARY(sdk STATIC
  ${SDK_SRC}
  ${PUGIXML}
  ${NET}
  ${HTTP}
  ${DIRTY_SEARCHER}
)
TARGET_LINK_LIBRARIES(sdk lz4 message pthread rt)
TARGET_COMPILE_OPTIONS(sdk PRIVATE -O2)
