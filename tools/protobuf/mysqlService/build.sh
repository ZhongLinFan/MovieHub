#!/bin/bash

#proto编译
protoc --proto_path=../common/ --proto_path=./ --cpp_out=./ ./*.proto
