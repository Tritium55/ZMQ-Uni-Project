cmake_minimum_required(VERSION 3.5)
project(TKNRN-Praxis3)      # Project Name
set(CMAKE_C_STANDARD 11)

find_library(ZeroMQ zmq REQUIRED)

# include all src files here
set(DISTRIBUTOR_SOURCES
    src/distributor/main.c
    src/lib/encoder.c
    src/lib/linked_list.c
    src/lib/hashmap.c
)

set(WORKER_SOURCES
    src/worker/main.c
    src/lib/encoder.c
    src/lib/linked_list.c
    src/lib/hashmap.c
)

add_executable(zmq_distributor ${DISTRIBUTOR_SOURCES})
target_compile_options(zmq_distributor PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(zmq_distributor PRIVATE zmq pthread)

add_executable(zmq_worker ${WORKER_SOURCES})
target_compile_options(zmq_worker PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(zmq_worker PRIVATE zmq pthread)

# pack submission
set(CPACK_SOURCE_GENERATOR "TGZ")   # make .tar.gz
set(CPACK_SOURCE_IGNORE_FILES ${CMAKE_BINARY_DIR} /\\..*$ \\.pdf$ /build/)  # ignore build files
set(CPACK_VERBATIM_VARIABLES YES)
include(CPack)
