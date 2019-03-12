# DEPENDENCIES
##############

# Require at least Protobuf 3 for gNMI
find_package(Protobuf 3.0.0 REQUIRED) #official cmake module
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

message(STATUS "Using protobuf ${protobuf_VERSION}")

# Require at least gRPC 1.12.0, maybe earlier can be used

# Official gRPCConfig.cmake installed by gRPC only if compiled with cmake
# Use our own FindGRPC.cmake instead
find_package(GRPC 1.12.0 REQUIRED)

get_filename_component(gnmi_proto "gnmi.proto" ABSOLUTE)
get_filename_component(gnmi_ext_proto "gnmi_ext.proto" ABSOLUTE)

# GENERATION OF SOURCE FILES
############################

# Official generator
protobuf_generate_cpp(gnmi_proto_srcs gnmi_proto_hdrs ${gnmi_proto})
protobuf_generate_cpp(gnmi_ext_proto_srcs gnmi_ext_proto_hdrs ${gnmi_ext_proto})

# Custom generator: Official `protobuf_generate_cpp` can't use grpc plugins
PROTOBUF_GENERATE_GRPC_CPP(gnmi_grpc_srcs gnmi_grpc_hdrs ${gnmi_proto})
PROTOBUF_GENERATE_GRPC_CPP(gnmi_ext_grpc_srcs gnmi_ext_grpc_hdrs ${gnmi_ext_proto})

# LINK OPTIONS FOR GNMI-SERVER
##############################

# Include generated protobuf C++ files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

#Create a new library named gnmi
add_library(gnmi ${gnmi_grpc_srcs} ${gnmi_ext_grpc_srcs} ${gnmi_proto_srcs} ${gnmi_ext_proto_srcs})

#Link our new gnmi library with grpc and protobuf libraries
target_link_libraries(gnmi ${GRPCPP_LIBRARY} ${PROTOBUF_LIBRARIES})
