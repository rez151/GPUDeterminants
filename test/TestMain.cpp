//
// Created by dokole on 12/30/17.
//
#include "../src/Controller.h"
#include <climits>
void swap( long* m, int order, int* index, int row1, int row2){
    int aux=index[row1];
    index[row1]=index[row2];
    index[row2]=aux;
    if(row1!=row2) index[order]*=-1;
}

long get( long* m, int order, int* index, int row, int column){
    return m[index[row]*order+column];
}

void set( long* m, int order, int* index, int row, int column, long value){
    m[index[row]*order+column]=value;
}

void subtract( long* m, int order, int* index, int row1, int row2, long factor){
    for(int i=0; i<order; i++){
        long v=get(m,order,index,row2,i) - factor*get(m,order,index,row1,i);
        set(m,order,index,row2,i,v);
    }
}

long absLong(long v){
    return v<0 ? -v : v;
}

int pivotIndex( long* m, int order, int* index, int diag){
    int result= -1;
    long pivot=LONG_MAX;
    for(int i=diag; i<order; i++){
        long pivotCandidate=absLong(get(m, order,index, i,diag));
        if(pivotCandidate!=0 && pivotCandidate<pivot){
            pivot=pivotCandidate;
            result=i;
        }
    }
    return result;
}



int decrease( long* m, int order,int* index, int diag){
    int result=0;
    int pIdx=pivotIndex(m, order,index, diag);
    if(pIdx==-1) return result;
    long pivot=get(m,order,index,pIdx,diag);
    swap(m,order,index,diag,pIdx);

    for(int i=diag+1; i<order; i++){
        long factor = get(m,order,index,i,diag)/pivot;
        if(factor!=0){
            result=1;
            subtract(m,order,index,diag,i,factor);
        }
    }
    return result;
}

void eliminate( long* m, int order,int* index, int diag){
    while(0!=decrease(m,order,index,diag)){
        ;
    }
}

void triangulate( long* m, int order,int* index){
    for(int i=0; i<order; i++){
        eliminate(m,order,index,i);
    }
}

long determinant( long* m, int order,int* index){
    triangulate(m,order,index);
    long result=1;
    for(int i=0; i<order; i++){
        result*=get(m,order,index,i,i);
    }
    return index[order]*result;
}

long* getMatrix( long* ms, int order, int position){
    int offset = order*order * position;
    return ms+offset;
}

void determinants( long* ms,
                   const int order,
                   long* output,
                   int cell) {
    int index[order+1];
    for(int i=0; i<order;i++){
        index[i]=i;
    }
    index[order]=1;
    long* m=getMatrix(ms, order, cell);
    output[cell] =  determinant(m, order, index);
}

//
//  Sets all elements of a matrix to value.
//  The elements on the main diagonal are set to 0.
//
void fillMatrix(long *array, long value, int order) {
    for (int i = 0; i < order; i++){
        for (int j = 0; j < order; j++){
            if(i==j)
                array[order*i+j]=0;
            else
                array[order*i+j]=value;
        }
    }
}


// Fills all matrices of dimension ORDER which are stored in array.
// The number of matrices is assumend to be NUM_SAMPLES
// The i-th matrix in the array i initialized with value i, and 0
// on the main diagonal
//
void fillMatrices(long *matrix, int numSamples, int order) {
    for (int i = 0; i < numSamples; i++){
       fillMatrix(matrix+i*order*order,i, order);
    }
}


bool compare(long *resultsCPU, long *resultsGPU, int numSamples) {
    bool result = true;
    for(int i=0;i<numSamples; i++){
        if(resultsCPU[i]!=resultsGPU[i]){
            result = false;
            break;
        }
    }
    return result;
}


template<typename TimeT = std::chrono::microseconds>
struct measure
{
    template<typename F, typename ...Args>
    static typename TimeT::rep execution(F func, Args&&... args)
    {
        auto start = std::chrono::system_clock::now();

        // Now call the function with all the parameters you need.
        func(std::forward<Args>(args)...);

        auto duration = std::chrono::duration_cast< TimeT>
                (std::chrono::system_clock::now() - start);

        return duration.count();
    }
};

int order = 100;
int semples = 1000;

std::string outfile = "";

long* matricesGPU;
long* resultsGPU;

long* matricesCPU;
long* resultsCPU;

static void show_usage(std::string name)
{
    std::cerr << "Usage: " << name << " <option(s)>\n "
              << "\tOptions:\n"
              << "\t\t-h,--help\t\tShow this help message\n"
              << "\t\t-o,--order ORDER\tSpecify the order of inputmatrices. Default: "<<order<<"\n"
              << "\t\t-s,--sempels SEMPELS\tSpecify the number of inputmatrices. Default: "<<semples<<"\n"
                << "\t\t-out,--output \tOutput file\n"
              << std::endl;
    exit(1);
}

static void readArgs(int argc, char* argv[]){
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if ((arg == "-h") || (arg == "--help")) {
            show_usage(argv[0]);
        } else if ((arg == "-o") || (arg == "--order")) {
            if (i + 1 < argc) {
                order = std::stoi(argv[++i]);
            } else {
                std::cerr << "--order option requires one argument." << std::endl;
                exit(1);
            }
        } else if ((arg == "-s") || (arg == "--semples")) {
            if (i + 1 < argc) {
                semples = std::stoi(argv[++i]);
            } else {
                std::cerr << "--semples option requires one argument." << std::endl;
                exit(1);
            }
        } else if ((arg == "-out") || (arg == "--output")) {
            if (i + 1 < argc) {
                outfile = argv[++i];
            } else {
                std::cerr << "--output option requires one argument." << std::endl;
                exit(1);
            }
        }
    }
}

void calculateCPU(){
    for(int i=0; i<semples; i++){
        determinants( matricesCPU,order,resultsCPU,i);
    }
}

void writeToFile(std::string file){
    std::ofstream myfile;
    myfile.open (outfile);
    myfile << file;
    myfile.close();
}

void testTimes(){
    std::ostringstream stream;

    for(int i = 30; i <= 130 ; i+=10){
        order = i;
        Controller controller(order);
        for(int j = 100; j <= 1000000; j*=10){
            semples = j;
            std::cout<<"Order: "<<i<<std::endl;
            std::cout<<"semples: "<<j<<std::endl;
            stream<<i<<",";
            stream<<j<<",";
            matricesCPU = new long[order * order * semples];
            resultsCPU = new long[semples];
            matricesGPU = new long[order * order * semples];
            resultsGPU = new long[semples];
            fillMatrices(matricesGPU, semples, order);
            fillMatrices(matricesCPU, semples, order);
            controller.initNumberSamples(semples);
            stream <<  measure<>::execution([&controller]() { controller.calculate(matricesGPU, resultsGPU); })
                      << ",";
            stream <<  measure<>::execution([]() { calculateCPU(); })
                      << ",";
            if (compare(resultsCPU, resultsGPU, semples)) {
                stream << "equal" << std::endl;
            } else {
                stream << "ERROR! " << std::endl;
                break;
            }
        }
    }
    writeToFile(stream.str());
}

int main(int argc, char* argv[]) {
    readArgs(argc,argv);
    if(outfile == "") {
        matricesCPU = new long[order * order * semples];
        resultsCPU = new long[semples];
        matricesGPU = new long[order * order * semples];
        resultsGPU = new long[semples];
        fillMatrices(matricesGPU, semples, order);
        fillMatrices(matricesCPU, semples, order);
        Controller controller(order);
        controller.initNumberSamples(semples);
        std::cout << "GPU execution time:"
                  << measure<>::execution([&controller]() { controller.calculate(matricesGPU, resultsGPU); })
                  << std::endl;
        std::cout << "CPU execution time:"
                  << measure<>::execution([]() { calculateCPU(); })
                  << std::endl;

        /*for(int i = 0; i< semples;i++){
            std::cout<<"GPU "<<i<<": "<<resultsGPU[i]<<std::endl;
            std::cout<<"CPU "<<i<<": "<<resultsCPU[i]<<std::endl;
        }*/
        if (compare(resultsCPU, resultsGPU, semples)) {
            std::cout << "Results equal" << std::endl;
        } else {
            std::cout << "ERROR! Results not equal" << std::endl;
        }
    } else{
        testTimes();
    }
}