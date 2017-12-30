//
// Created by dokole on 12/30/17.
//

#ifndef GPUDETERMINANTS_CONTROLLER_H
#define GPUDETERMINANTS_CONTROLLER_H
#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_MINIMUM_OPENCL_VERSION 110
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define __CL_ENABLE_EXCEPTIONS 1

#define MAX_ORDER 130
#define AVAILABLE_DEVICES 2

#include <CL/cl2.hpp>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include "GPUSliderule.h"

class Controller{
    GPUSliderule sliderules[AVAILABLE_DEVICES];
    std::vector<cl::Device> all_devices;
    std::string code;
    const std::string codeFilePath = "determinants.cl";

    int order;
    int numSemples;

    void readKernelCode();
public:
    Controller(int order);
    void initOrder(int order);
    void initNumberSamples(int numSemples);
    void calculate(long* matrices, long* results);
    void calculate(long* matrices, long* results,int numSemples);
};

#endif //GPUDETERMINANTS_CONTROLLER_H
