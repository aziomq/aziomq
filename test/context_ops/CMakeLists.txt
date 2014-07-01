#include_directories("${PROJECT_SOURCE_DIR}/src")
add_executable(test_context_ops main.cpp)
target_link_libraries(test_context_ops ${Boost_LIBRARIES}
                                       ${ZeroMQ_LIBRARIES}
                                       aziomq)
