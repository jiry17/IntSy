cmake_minimum_required(VERSION 3.5)
project(IntSy)

link_directories(Z3PATH/build)
include_directories(Z3PATH/src/api Z3PATH/src/api/c++)
set(Z3_FILE libz3.so)

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2")


include_directories('./lib ./circuit ./solver ./parser')

add_library(basic_lib lib/value.h lib/rule.h lib/vsa.cpp lib/vsa.h lib/non_terminal.h lib/specification.h lib/example_space.h lib/example_space.cpp lib/log_util.h lib/config.h lib/config.cpp)
add_library(rule_lib circuit/circuit_rule.cpp circuit/circuit_rule.h int/int_rule.cpp int/int_rule.h string/string_rule.cpp string/string_rule.h)
add_library(solver_lib solver/solver.h solver/solver.cpp solver/solver.h solver/sampler.cpp solver/sampler.h solver/vsa_encoder.cpp solver/vsa_encoder.h)
add_library(parser_lib parser/smtlib_parser.h parser/smtlib_parser.cpp)

add_executable(main main.cpp)
target_link_libraries(main solver_lib parser_lib basic_lib rule_lib ${Z3_FILE} glog)
