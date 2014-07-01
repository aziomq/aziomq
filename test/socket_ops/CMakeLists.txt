#include_directories("${PROJECT_SOURCE_DIR}/src")
add_executable(test_socket_ops main.cpp)
target_link_libraries(test_socket_ops ${Boost_LIBRARIES}
                                      ${ZeroMQ_LIBRARIES}
                                      aziomq)
