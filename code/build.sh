#!/bin/bash

PROJECT_ROOT=`pwd`

# aws-sdk-cpp
mkdir -p $PROJECT_ROOT/build/aws-sdk-cpp/
cd $PROJECT_ROOT/build/aws-sdk-cpp/
cmake $PROJECT_ROOT/lib/aws-sdk-cpp -DBUILD_ONLY="s3" -DAUTORUN_UNIT_TESTS=OFF
make
make install

# aws-lambda-cpp
mkdir $PROJECT_ROOT/lib/aws-lambda-cpp/build
cd $PROJECT_ROOT/lib/aws-lambda-cpp/build
cmake .. -DCMAKE_BUILD_TYPE=Release
make 
make install

# build the project :)
cd $PROJECT_ROOT/build
cmake ..
make
make aws-lambda-package-quote
make aws-lambda-package-create_dictionary
make aws-lambda-package-create_quote
