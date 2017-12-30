//
// Created by dokole on 12/30/17.
//
#include "Controller.h"

Controller::Controller(int order) {
    //Find Platform
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if (all_platforms.size()==0) {
        std::cout << "No platforms found. Check OpenCL installation!\n";
        exit(1);
    }
    //Find Devices
    all_platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
    if(all_devices.size()==0){
        std::cout << "No devices found. Check OpenCL installation!\n";
        exit(1);
    }
    for(int i = 0; i< AVAILABLE_DEVICES; i++){
        this->sliderules[i] = GPUSliderule(&(this->all_devices[i]),i);
    }
    this->initOrder(order);
}

void Controller::initOrder(int order) {
    this->order = order;
    this->readKernelCode();
    for(int i = 0; i< AVAILABLE_DEVICES; i++){
        this->sliderules[i].build(this->code,this->order);
    }
}

void Controller::readKernelCode() {
    std::ostringstream oss;
    oss << "#define ORDER "<<this->order<<std::endl;
    std::ifstream infile(this->codeFilePath);
    std::string line;
    while (std::getline(infile, line))
    {
        oss << line<<std::endl;
    }
    this->code = oss.str();
}

void Controller::initNumberSamples(int numSemples) {
    this->numSemples = numSemples;
    /*for(int i = 0; i< AVAILABLE_DEVICES; i++){
        this->sliderules[i].setNumSemples(numSemples);
    }*/
}

void run(GPUSliderule* sliderule, long *matrices, long *results){
    sliderule->caluclate(matrices,results);
}

void Controller::calculate(long *matrices, long *results) {
    int tmp = this->numSemples/AVAILABLE_DEVICES;
    std::thread threads[AVAILABLE_DEVICES];
    long* matricesPtr = matrices;
    long* resultsPtr = results;
    for(int i = 0; i< AVAILABLE_DEVICES; i++){
        if(i == AVAILABLE_DEVICES-1){
            tmp = this->numSemples - i*tmp;
        }
        matricesPtr = i*tmp*this->order*this->order+matricesPtr;
        resultsPtr = i*tmp+resultsPtr;
        this->sliderules[i].setNumSemples(tmp);
        threads[i] = std::thread(run,&(this->sliderules[i]),matricesPtr,resultsPtr);
    }
    for(int i = 0; i< AVAILABLE_DEVICES; i++){
        threads[i].join();
    }
}

void Controller::calculate(long *matrices, long *results, int numSemples) {
    this->initNumberSamples(numSemples);
    this->calculate(matrices,results);
}
