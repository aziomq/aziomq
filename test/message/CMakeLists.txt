add_executable(test_message main.cpp)
target_link_libraries(test_message ${Boost_LIBRARIES}
                                   ${ZeroMQ_LIBRARIES}
                                   aziomq)

add_test(message ${CMAKE_CURRENT_BINARY_DIR}/test_message)

