//
// Created by dokole on 12/30/17.
//

#ifndef GPUDETERMINANTS_GPUSLIDERULE_H
#define GPUDETERMINANTS_GPUSLIDERULE_H
#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_MINIMUM_OPENCL_VERSION 110
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define __CL_ENABLE_EXCEPTIONS 1

#include "LoadBalancer.h"
#include <iostream>
#include <CL/cl2.hpp>
class GPUSliderule{
    cl::CommandQueue queues[2];
    cl::Kernel kernels[2];
    cl::Buffer inputBuffers[2];
    cl::Buffer outputBuffers[2];
    cl::Context context;
    LoadBalancer* balancer;
    int currentId = 0;
    void next();
    int index;
public:
    GPUSliderule();
    GPUSliderule(cl::Device* devicePtr,int index);
    void build(std::string code, int order);
    void setNumSemples(int numSemples);
    void caluclate(long* matrices, long* results);
};
#endif //GPUDETERMINANTS_GPUSLIDERULE_H
