find_package(OpenCL 1.2 REQUIRED)
find_package(Threads REQUIRED)
set(HEADER_FILES ../src/Controller.h ../src/GPUSliderule.h ../src/LoadBalancer.h)
set(SRC_FILES  Controller.cpp GPUSliderule.cpp LoadBalancer.cpp)
add_library(GPUSliderule ${HEADER_FILES} ${SRC_FILES})



target_link_libraries(GPUSliderule Threads::Threads OpenCL)