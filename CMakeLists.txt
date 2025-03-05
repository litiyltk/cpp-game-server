cmake_minimum_required(VERSION 3.11)

project(game_server CXX)

# Для включения всех предупреждений и ошибок
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror -fno-stack-limit")
# Для дополнительных диагностик, которые могут помочь при поиске проблем
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")  # Генерация отладочной информации без оптимизаций

set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Добавляем библиотеку, указывая, что она статическая.
add_library(game_server_lib STATIC
	src/model.h
    src/model.cpp
    src/loot_generator.h
    src/loot_generator.cpp
    src/tagged.h
	src/geom.h
	src/collision_detector.h
	src/collision_detector.cpp
	src/model_serialization.h
	src/model_serialization.cpp
)
target_include_directories(game_server_lib PUBLIC CONAN_PKG::boost)
target_link_libraries(game_server_lib PUBLIC CONAN_PKG::boost Threads::Threads CONAN_PKG::libpq CONAN_PKG::libpqxx)


add_executable(game_server
	src/main.cpp
	src/http_server.h
	src/http_server.cpp
	src/boost_json.cpp
	src/sdk.h
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.h
	src/request_handler.cpp
	src/logging_request_handler.h
	src/response_m.h
	src/response_m.cpp
	src/magic_defs.h
	src/server_logger.h
	src/server_logger.cpp
	src/app.h
	src/app.cpp
	src/app_serialization.h
	src/app_serialization.cpp
	src/api_handler.h
	src/api_handler.cpp
	src/state_saver.h
	src/state_saver.cpp
	src/postgres.h
	src/postgres.cpp
	src/tagged_uuid.h
	src/tagged_uuid.cpp
	src/ticker.h
	src/ticker.cpp
)

add_executable(game_server_tests
    tests/model_tests.cpp
	tests/collision_detector_tests.cpp
    tests/loot_generator_tests.cpp
	tests/state_serialization_tests.cpp
)

target_link_libraries(game_server game_server_lib)
target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 game_server_lib)

include(CTest)
include(${CONAN_BUILD_DIRS_CATCH2}/Catch.cmake)
catch_discover_tests(game_server_tests)