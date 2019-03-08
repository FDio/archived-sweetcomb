set(protobuf_MODULE_COMPATIBLE TRUE)

find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "Using protobuf ${protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF ${PROTOBUF_LIBRARIES})
set(_PROTOBUF_PROTOC ${PROTOBUF_PROTOC_EXECUTABLE})

find_package(gRPC CONFIG REQUIRED)
set(_GRPC_GRPCPP gRPC::grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)


set(PROTOS
    gnmi.proto
    gnmi_ext.proto
)

get_filename_component(hw_proto "gnmi.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)
get_filename_component(gnmi_ext_proto "gnmi_ext.proto" ABSOLUTE)
get_filename_component(gnmi_ext_proto_path "${gnmi_ext_proto}" PATH)

# Generated sources
set(hw_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/gnmi.pb.cc")
set(hw_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/gnmi.pb.h")
set(hw_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/gnmi.grpc.pb.cc")
set(hw_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/gnmi.grpc.pb.h")
set(gnmi_ext_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/gnmi_ext.pb.cc")
set(gnmi_ext_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/gnmi_ext.pb.h")
set(gnmi_ext_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/gnmi_ext.grpc.pb.cc")
set(gnmi_ext_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/gnmi_ext.grpc.pb.h")

add_custom_command(
      OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}" "${gnmi_ext_proto_srcs}" "${gnmi_ext_proto_hdrs}" "${gnmi_ext_grpc_srcs}" "${gnmi_ext_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path}" "${gnmi_ext_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto}" "${gnmi_ext_proto}"
      DEPENDS "${hw_proto}" "${gnmi_ext_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(proto ${hw_proto_srcs} ${hw_grpc_srcs}
                                ${gnmi_ext_proto_srcs} ${gnmi_ext_grpc_srcs})
target_link_libraries(proto ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF})

