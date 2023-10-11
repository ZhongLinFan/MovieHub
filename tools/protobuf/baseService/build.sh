#!/bin/bash

#proto编译
#因为必须要用到user的结构体，所以必须要嵌套的使用proto协议
#这里为啥这样写path，看base协议对应的那个网址
protoc -I=../common -I =./  --cpp_out=./ ./*.proto
