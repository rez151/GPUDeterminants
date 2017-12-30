//
// Created by dokole on 12/30/17.
//

#ifndef GPUDETERMINANTS_LOADBALANCER_H
#define GPUDETERMINANTS_LOADBALANCER_H
#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_MINIMUM_OPENCL_VERSION 110
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define __CL_ENABLE_EXCEPTIONS 1
#include <CL/cl2.hpp>
class LoadBalancer{
    long allocateableMemory;
    int order;
    int numSemples = 0;
    int maxSemplesInQueue;

    long* matrices;
    long* results;

    int counter;
public:
    size_t matrixSize;
    cl::Device* devicePtr;
    LoadBalancer();
    LoadBalancer(cl::Device* devicePtr);
    void setOrder(int order);
    void setPtr(long* matrices,long* results);
    void setNumSemples(int numSemples);
    bool hasNext();
    long* getMatricesPtr();
    long* getResultsPtr();
    void next();
    int getMatrixCount();
};
#endif //GPUDETERMINANTS_LOADBALANCER_H
