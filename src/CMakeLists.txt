cmake_minimum_required(VERSION 2.8)

set (CMAKE_INSTALL_PREFIX /usr/local)
message("CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX} (should be /usr/local)")

project(rdstools)
message("CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX} (should be /usr/local)")

add_executable(hex2rds "hex2rds.c")

add_executable(srdsgrep "srdsgrep.c")

add_executable(srdsmerge "srdsmerge.c")

install(TARGETS hex2rds srdsgrep srdsmerge DESTINATION bin )
