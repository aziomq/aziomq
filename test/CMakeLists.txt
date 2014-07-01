add_subdirectory(message)
add_subdirectory(context_ops)
add_subdirectory(socket_ops)
add_subdirectory(socket)
    #include_directories("${PROJECT_SOURCE_DIR}/src")
#add_executable(test_aziomq main.cpp)
#target_link_libraries(test_aziomq ${Boost_LIBRARIES}
                                  #${ZeroMQ_LIBRARIES}
                                  #aziomq)

#add_test(message ${CMAKE_CURRENT_BINARY_DIR}/test_context_ops)
#add_test(message ${CMAKE_CURRENT_BINARY_DIR}/test_socket_ops)
#add_test(message ${CMAKE_CURRENT_BINARY_DIR}/test_socket)



