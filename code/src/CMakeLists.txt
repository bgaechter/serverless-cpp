add_executable(quote main.cpp)

target_link_libraries(quote cpp_redis
                        AWS::aws-lambda-runtime
                        ${AWSSDK_LINK_LIBRARIES})

add_executable(create_dictionary create_dictionary.cpp)

target_link_libraries(create_dictionary cpp_redis
                        AWS::aws-lambda-runtime
                        ${AWSSDK_LINK_LIBRARIES})

add_executable(create_quote create_quote.cpp)

target_link_libraries(create_quote cpp_redis
                        AWS::aws-lambda-runtime
                        ${AWSSDK_LINK_LIBRARIES})