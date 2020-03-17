cmake_minimum_required(VERSION 3.10)
project(emeralddb)
#the version number
set(emeralddb_VERSION_MAJOR 1)
set(emeralddb_VERSION_MINOR 0)

#set build model
SET(CMAKE_BUILD_TYPE Debug)

#configure a header file to pass some of the CMake settings
#to the source code
#configure_file(
#    ${PROJECT_SOURCE_DIR}/src/helloConfig.h.in
#    ${PROJECT_SOURCE_DIR}/src/helloConfig.h
#)

#set headfile path

#set execute path
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#set library path
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

add_executable(emeralddb
    src/pmd/pmdMain.cpp src/pmd/pmdOptions.cpp src/pmd/pmdTcpListener.cpp
    src/pmd/pmd.cpp src/pmd/pmdEDU.cpp src/pmd/pmdEDUMgr.cpp src/pmd/pmdAgent.cpp
    src/bson/src/bsonobj.cpp src/bson/src/util/json.cpp src/bson/src/oid.cpp
    src/bson/src/lib/base64.cpp src/bson/src/lib/md5.cpp src/bson/src/lib/nonce.cpp
    src/oss/ossSocket.cpp src/oss/ossPrimitiveFileOp.cpp   src/oss/ossMmapFile.cpp
    src/oss/ossHash.cpp
    src/pd/pd.cpp   src/msg/msg.cpp src/dms/dms.cpp src/rtn/rtn.cpp src/ixm/ixmBucket.cpp
    src/mon/monCB.cpp
)

add_executable(edb
    src/client/edb.cpp src/client/command.cpp src/client/commandFactory.cpp 
    src/bson/src/bsonobj.cpp src/bson/src/util/json.cpp src/bson/src/oid.cpp 
    src/bson/src/lib/base64.cpp src/bson/src/lib/md5.cpp src/bson/src/lib/nonce.cpp 
    src/oss/ossSocket.cpp src/oss/ossPrimitiveFileOp.cpp 
    src/pd/pd.cpp   src/msg/msg.cpp
)

target_include_directories(emeralddb
    PRIVATE
    ${PROJECT_SOURCE_DIR}/../emeralddb-local/emeralddb/boost
    ${PROJECT_SOURCE_DIR}/src/bson/src
    ${PROJECT_SOURCE_DIR}/include
)

target_include_directories(edb
    PRIVATE
    ${PROJECT_SOURCE_DIR}/../emeralddb-local/emeralddb/boost
    ${PROJECT_SOURCE_DIR}/src/bson/src
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_options(emeralddb
    PRIVATE
    -ggdb 
    -Wall 
    -O0
)

target_compile_options(edb
    PRIVATE
    -ggdb
    -Wall
    -O0
)

target_compile_definitions(emeralddb
    PRIVATE
    FILE_OFFSET_BITS=64
)


target_compile_definitions(edb
    PRIVATE
    FILE_OFFSET_BITS=64
)

target_link_libraries(emeralddb
    PRIVATE
    pthread 
    m 
    boost_system 
    boost_thread 
    boost_program_options
    rt
)

target_link_libraries(edb
    PRIVATE
    pthread
    m
    boost_system
    boost_thread
    boost_program_options
    rt
)

target_link_libraries(emeralddb
    PRIVATE
    -fPIC
    -rdynamic
    -L${PROJECT_SOURCE_DIR}/../emeralddb-local/emeralddb/boost/stage/lib
    -pthread
)

target_link_libraries(edb
    PRIVATE
    -fPIC
    -rdynamic
    -L${PROJECT_SOURCE_DIR}/../emeralddb-local/emeralddb/boost/stage/lib
    -pthread
)

