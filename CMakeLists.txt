cmake_minimum_required(VERSION 3.8)
project(simple_mail)

set(CMAKE_C_STANDARD 11)

include(FindPkgConfig)

pkg_search_module(GLIB REQUIRED glib-2.0)
include_directories(${GLIB_INCLUDE_DIRS})

pkg_search_module(GMIME REQUIRED gmime-3.0)
include_directories(${GMIME_INCLUDE_DIRS})

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

set(CONN_SOURCE_FILES conn/conn.c conn/conn.h conn/error.h conn/op.c conn/log.h client/main.c)
add_library(conn STATIC ${CONN_SOURCE_FILES})
include_directories(conn)
target_link_libraries(conn ${OPENSSL_LIBRARIES} ${GLIB_LIBRARIES})

set(SMTP_SOURCE_FILES smtp/smtp.h smtp/parser.c smtp/error.h
        smtp/parser.h smtp/client.c smtp/client.h smtp/log.h)
add_library(smtp STATIC ${SMTP_SOURCE_FILES})
include_directories(smtp)
target_link_libraries(smtp conn ${GLIB_LIBRARIES})

set(POP3_SOURCE_FILES pop3/client.c pop3/client.h pop3/log.h pop3/pop3.h pop3/error.h)
add_library(pop3 STATIC ${POP3_SOURCE_FILES})
include_directories(pop3)
target_link_libraries(pop3 conn ${GLIB_LIBRARIES})

set(SOURCE_FILES client/main.c client/common.h client/send.c client/recv.c)
add_executable(simple-mail ${SOURCE_FILES})
add_dependencies(simple-mail argparse)
target_link_libraries(simple-mail smtp pop3 m ${CMAKE_BINARY_DIR}/libargparse.a ${GMIME_LIBRARIES} ${GLIB_LIBRARIES})

include_directories(vendor/argparse)
add_custom_target(argparse
        COMMAND cd ${CMAKE_SOURCE_DIR}/vendor/argparse &&
                make libargparse.a &&
                cp libargparse.a ${CMAKE_BINARY_DIR})

add_library(greatest INTERFACE)
target_sources(greatest INTERFACE ${PROJECT_SOURCE_DIR}/vendor/greatest/greatest.h)

set(TEST_FILES test/smtp_parser.h test/run.c test/conn_op.h)
add_executable(simple_mail_test ${TEST_FILES})
target_include_directories(simple_mail_test PRIVATE vendor/greatest)
target_link_libraries(simple_mail_test smtp pop3 conn greatest ${GLIB_LIBRARIES})