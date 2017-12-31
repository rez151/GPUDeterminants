//
// Created by dokole on 12/30/17.
//
#include <iostream>
#include "LoadBalancer.h"

LoadBalancer::LoadBalancer() {}

LoadBalancer::LoadBalancer(cl::Device *devicePtr) {
    this->devicePtr = devicePtr;
    this->allocateableMemory=this->devicePtr->getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>();
}

void LoadBalancer::setOrder(int order) {
    this->order = order;
    this->matrixSize = this->order*this->order;
    long tmp = this->allocateableMemory/(sizeof(long)*this->matrixSize);
    if(tmp*sizeof(long)>this->allocateableMemory-sizeof(long)*this->matrixSize*tmp){
        tmp = (this->allocateableMemory-tmp*sizeof(long))/(sizeof(long)*this->matrixSize);
    }
    this->maxSemplesInQueue = tmp;
}

void LoadBalancer::setNumSemples(int numSemples) {
    this->numSemples = numSemples;
}

int LoadBalancer::getMatrixCount() {
    if(this->numSemples-this->counter > this->maxSemplesInQueue){
        return this->maxSemplesInQueue;
    }
    return this->numSemples-this->counter;
}

void LoadBalancer::next() {
    this->counter += this->getMatrixCount();
}

bool LoadBalancer::hasNext() {
    return this->counter<this->numSemples;
}

long* LoadBalancer::getMatricesPtr() {
    return this->matrices+this->matrixSize*this->counter;
}

long* LoadBalancer::getResultsPtr() {
    return this->results+this->counter;
}

void LoadBalancer::setPtr(long *matrices, long *results) {
    this->counter = 0;
    this->matrices = matrices;
    this->results = results;
}