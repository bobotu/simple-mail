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
target_link_libraries(conn ${OPENSSL_LIBRARIES})

set(SMTP_SOURCE_FILES smtp/smtp.h smtp/parser.c smtp/error.h
        smtp/parser.h smtp/client.c smtp/client.h smtp/log.h)
add_library(smtp STATIC ${SMTP_SOURCE_FILES})
include_directories(smtp)
target_link_libraries(smtp conn ${GLIB_LIBRARIES} ${GMIME_LIBRARIES})

set(POP3_SOURCE_FILES pop3/client.c pop3/client.h pop3/log.h pop3/pop3.h pop3/error.h)
add_library(pop3 STATIC ${POP3_SOURCE_FILES})
include_directories(pop3)
target_link_libraries(pop3 conn ${GLIB_LIBRARIES})

set(SOURCE_FILES client/main.c)
add_executable(simple_mail ${SOURCE_FILES})
target_link_libraries(simple_mail smtp pop3 ${GMIME_LIBRARIES} ${GLIB_LIBRARIES})

add_library(greatest INTERFACE)
target_sources(greatest INTERFACE ${PROJECT_SOURCE_DIR}/vendor/greatest/greatest.h)

set(TEST_FILES test/smtp_parser.h test/run.c test/conn_op.h)
add_executable(simple_mail_test ${TEST_FILES})
target_include_directories(simple_mail_test PRIVATE vendor/greatest)
target_link_libraries(simple_mail_test smtp pop3 conn greatest ${GLIB_LIBRARIES})