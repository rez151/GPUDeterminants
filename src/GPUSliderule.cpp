//
// Created by dokole on 12/30/17.
//
#include "GPUSliderule.h"

GPUSliderule::GPUSliderule() {
}

GPUSliderule::GPUSliderule(cl::Device *devicePtr,int index) {
    this->index = index;
    this->balancer = new LoadBalancer(devicePtr);
    this->context = cl::Context({*devicePtr});
    for(int i = 0; i < 2; i++){
        this->queues[i] = cl::CommandQueue(this->context, *devicePtr);
    }
}

void GPUSliderule::build(std::string code, int order){
    this->balancer->setOrder(order);
    cl::Program::Sources sources;
    sources.push_back({code.c_str(), code.length()});
    cl::Program program(this->context, sources);
    if (program.build({*(this->balancer->devicePtr)}) != CL_SUCCESS) {
        std::cout << "Error building: "
                  << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(*(this->balancer->devicePtr))
                  << std::endl;
        exit(-1);
    }
    for (int i = 0; i < 2; i++) {
        this->kernels[i] = cl::Kernel(program, "determinants");
    }
}

void GPUSliderule::setNumSemples(int numSemples) {
    this->balancer->setNumSemples(numSemples);
}

void GPUSliderule::caluclate(long *matrices, long *results) {
    this->balancer->setPtr(matrices,results);
    //
    do {
        this->queues[this->currentId].finish();
        this->inputBuffers[this->currentId]
                = cl::Buffer(this->context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                             sizeof(long)*this->balancer->matrixSize * this->balancer->getMatrixCount(),
                             this->balancer->getMatricesPtr());
        this->queues[this->currentId].enqueueWriteBuffer(
                this->inputBuffers[this->currentId], CL_TRUE, 0,
                sizeof(long)*this->balancer->matrixSize * this->balancer->getMatrixCount(),
                this->balancer->getMatricesPtr());

        this->kernels[this->currentId].setArg(0, this->inputBuffers[this->currentId]);
        this->outputBuffers[this->currentId]
                = cl::Buffer(this->context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                             sizeof(long) * this->balancer->getMatrixCount(),
                             this->balancer->getResultsPtr());
        this->kernels[this->currentId].setArg(1, this->outputBuffers[this->currentId]);
        this->queues[this->currentId].enqueueNDRangeKernel(
                this->kernels[this->currentId], cl::NullRange,
                cl::NDRange(this->balancer->getMatrixCount()));
        this->queues[this->currentId].enqueueReadBuffer(
                this->outputBuffers[this->currentId], CL_FALSE, 0,
                sizeof(long) * this->balancer->getMatrixCount(), this->balancer->getResultsPtr());
        this->next();
    }while (this->balancer->hasNext());
    this->queues[0].finish();
    this->queues[1].finish();
}

void GPUSliderule::next() {
    this->currentId == 0 ? this->currentId = 1:this->currentId = 0;
    this->balancer->next();
}
