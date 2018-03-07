//
// Created by dokole on 12/30/17.
//
//
#include "../src/Controller.h"
#include <climits>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */

//Socketimports
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <arpa/inet.h>

//string funktions
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost::interprocess;

#define MAX_BUF 1024

int sockfd;
int newsockfd;
int portno = 51717;
int clilen;
char buffer[256];
struct sockaddr_in serv_addr, cli_addr;
int n;


void swap(long *m, int order, int *index, int row1, int row2) {
    int aux = index[row1];
    index[row1] = index[row2];
    index[row2] = aux;
    if (row1 != row2) index[order] *= -1;
}

long get(long *m, int order, int *index, int row, int column) {
    return m[index[row] * order + column];
}

void set(long *m, int order, int *index, int row, int column, long value) {
    m[index[row] * order + column] = value;
}

void subtract(long *m, int order, int *index, int row1, int row2, long factor) {
    for (int i = 0; i < order; i++) {
        long v = get(m, order, index, row2, i) - factor * get(m, order, index, row1, i);
        set(m, order, index, row2, i, v);
    }
}

long absLong(long v) {
    return v < 0 ? -v : v;
}

int pivotIndex(long *m, int order, int *index, int diag) {
    int result = -1;
    long pivot = LONG_MAX;
    for (int i = diag; i < order; i++) {
        long pivotCandidate = absLong(get(m, order, index, i, diag));
        if (pivotCandidate != 0 && pivotCandidate < pivot) {
            pivot = pivotCandidate;
            result = i;
        }
    }
    return result;
}


int decrease(long *m, int order, int *index, int diag) {
    int result = 0;
    int pIdx = pivotIndex(m, order, index, diag);
    if (pIdx == -1) return result;
    long pivot = get(m, order, index, pIdx, diag);
    swap(m, order, index, diag, pIdx);

    for (int i = diag + 1; i < order; i++) {
        long factor = get(m, order, index, i, diag) / pivot;
        if (factor != 0) {
            result = 1;
            subtract(m, order, index, diag, i, factor);
        }
    }
    return result;
}

void eliminate(long *m, int order, int *index, int diag) {
    while (0 != decrease(m, order, index, diag)) { ;
    }
}

void triangulate(long *m, int order, int *index) {
    for (int i = 0; i < order; i++) {
        eliminate(m, order, index, i);
    }
}

long determinant(long *m, int order, int *index) {
    triangulate(m, order, index);
    long result = 1;
    for (int i = 0; i < order; i++) {
        result *= get(m, order, index, i, i);
    }
    return index[order] * result;
}

long *getMatrix(long *ms, int order, int position) {
    int offset = order * order * position;
    return ms + offset;
}

void determinants(long *ms,
                  const int order,
                  long *output,
                  int cell) {
    int index[order + 1];
    for (int i = 0; i < order; i++) {
        index[i] = i;
    }
    index[order] = 1;
    long *m = getMatrix(ms, order, cell);
    output[cell] = determinant(m, order, index);
}

//
//  Sets all elements of a matrix to value.
//  The elements on the main diagonal are set to 0.
//
void fillMatrix(long *array, long value, int order) {
    for (int i = 0; i < order; i++) {
        for (int j = 0; j < order; j++) {
            if (i == j)
                array[order * i + j] = 0;
            else
                array[order * i + j] = value;
        }
    }
}


// Fills all matrices of dimension ORDER which are stored in array.
// The number of matrices is assumend to be NUM_SAMPLES
// The i-th matrix in the array i initialized with value i, and 0
// on the main diagonal
//
void fillMatrices(long *matrix, int numSamples, int order) {
    for (int i = 0; i < numSamples; i++) {
        fillMatrix(matrix + i * order * order, i, order);
    }
}


bool compare(long *resultsCPU, long *resultsGPU, int numSamples) {
    bool result = true;
    for (int i = 0; i < numSamples; i++) {
        if (resultsCPU[i] != resultsGPU[i]) {
            result = false;
            break;
        }
    }
    return result;
}


template<typename TimeT = std::chrono::microseconds>
struct measure {
    template<typename F, typename ...Args>
    static typename TimeT::rep execution(F func, Args &&... args) {
        auto start = std::chrono::system_clock::now();

        // Now call the function with all the parameters you need.
        func(std::forward<Args>(args)...);

        auto duration = std::chrono::duration_cast<TimeT>
                (std::chrono::system_clock::now() - start);

        return duration.count();
    }
};


int order = 10;
int semples = 10;

std::string outfile = "";

long *matricesGPU;
long *resultsGPU;

long *matricesCPU;
long *resultsCPU;

static void show_usage(std::string name) {
    std::cerr << "Usage: " << name << " <option(s)>\n "
              << "\tOptions:\n"
              << "\t\t-h,--help\t\tShow this help message\n"
              << "\t\t-o,--order ORDER\tSpecify the order of inputmatrices. Default: " << order << "\n"
              << "\t\t-s,--sempels SEMPELS\tSpecify the number of inputmatrices. Default: " << semples << "\n"
              << "\t\t-out,--output \tOutput file\n"
              << std::endl;
    exit(1);
}

static void readArgs(int argc, char *argv[]) {
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

void calculateCPU() {
    for (int i = 0; i < semples; i++) {
        determinants(matricesCPU, order, resultsCPU, i);
    }
}

void writeToFile(std::string file) {
    std::ofstream myfile;
    myfile.open(outfile);
    myfile << file;
    myfile.close();
}

void testTimes() {
    std::ofstream stream;
    stream.open(outfile);
    Controller controller(order);
    for (int i = 10; i <= 130; i += 10) {
        order = i;
        controller.initOrder(order);
        for (int j = 100; j <= 100000; j *= 10) {
            semples = j;
            std::cout << "Order: " << i << std::endl;
            std::cout << "semples: " << j << std::endl;
            stream << i << ",";
            stream << j << ",";
            matricesCPU = new long[order * order * semples];
            resultsCPU = new long[semples];
            matricesGPU = new long[order * order * semples];
            resultsGPU = new long[semples];
            fillMatrices(matricesGPU, semples, order);
            fillMatrices(matricesCPU, semples, order);
            controller.initNumberSamples(semples);
            stream << measure<>::execution([&controller]() { controller.calculate(matricesGPU, resultsGPU); })
                   << ",";
            stream << measure<>::execution([]() { calculateCPU(); })
                   << ",";
            if (compare(resultsCPU, resultsGPU, semples)) {
                stream << "equal" << std::endl;
            } else {
                stream << "ERROR! " << std::endl;
                break;
            }
            free(matricesCPU);
            free(matricesGPU);
            free(resultsCPU);
            free(resultsGPU);
        }
    }
    stream.close();
    //writeToFile(stream.str());
}

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void readFifo(char *file, int fifonr) {
    int proc;
    char buf[MAX_BUF];

    proc = open(file, O_RDONLY);
    read(proc, buf, MAX_BUF);
    printf("Received: %s\n", buf);
    close(proc);
}

void initSocket() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd, 50);
    clilen = sizeof(cli_addr);

}

void writeToSocket(char *message) {

    std::cout << "sending to client: " << message << std::endl;

    bzero(buffer, 256);
    n = write(newsockfd, message, strlen(message));
    if (n < 0)
        error("ERROR writing to socket");
}

char *readFromSocket() {
    bzero(buffer, 256);
    n = read(newsockfd, buffer, 255);
    if (n < 0) {
        error("ERROR reading from socket");
        return "error";
    } else {
        std::cout << "received from client: " << buffer << std::endl;
        return buffer;
    }

}

void printSharedMemoryMat(long *arrayptr) {

    for (int i = 1; i <= order * order * semples; i++) {

        if (i % (order * order) == 0) {
            std::cout << arrayptr[i] << std::endl;
        } else {
            std::cout << arrayptr[i];
        }
    }
}

void printSharedMemoryRes(long *arrayptr) {

    for (int i = 0; i < semples; i++) {

        std::cout << arrayptr[i] << "  " << std::endl;
    }
}


int main(int argc, char *argv[]) {

/*
    if (daemon(0, 0) < 0) {
        perror("daemon");
        exit(2);
    }
    //Daemon from here on
    openlog("gpudeterm", LOG_PID, LOG_DAEMON);
*/

    unsigned long sizeMatrixElements = order * order * semples * sizeof(long);
    unsigned long sizeResultsElements = semples * sizeof(long);

    readArgs(argc, argv);
    if (outfile == "") {
        matricesCPU = new long[order * order * semples];
        resultsCPU = new long[semples];

        fillMatrices(matricesCPU, semples, order);

        Controller controller(order);
        controller.initNumberSamples(semples);

        std::cout << "Controler initialized" << std::endl;


        initSocket();

        //Remove shared memory on construction and destruction
/*        struct shm_remove {
            shm_remove() {
                shared_memory_object::remove("MatSharedMemory");
                shared_memory_object::remove("ResultsSharedMemory");
            }

            ~shm_remove() {
                //shared_memory_object::remove("MatSharedMemory");
                //shared_memory_object::remove("ResultsSharedMemory");
            }
        } remover;*/

        //Create a shared memory object for matricies and results
        shared_memory_object matriciesSHM(open_or_create, "MatSharedMemory", read_write);
        shared_memory_object resultSHM(open_or_create, "ResultsSharedMemory", read_write);

        //Set size
        matriciesSHM.truncate(sizeMatrixElements);
        resultSHM.truncate(sizeResultsElements);

        //Map the whole shared memory in this process
        mapped_region matRegion(matriciesSHM, read_only);
        mapped_region resultsRegion(resultSHM, read_write);

        long *matricesptr = (long *) matRegion.get_address();

        long *resultsptr = (long *) resultsRegion.get_address();

        while (true) {


            std::cout << "waiting for connection..." << std::endl;
            newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (unsigned int *) &clilen);
            if (newsockfd < 0) {
                error("ERROR on accept");
            }

            std::cout << "connected" << std::endl;


            char *message = readFromSocket();
            std::string msg(message);


            if (msg.compare("start") == 0) {
                std::cout << "start" << std::endl;

                printSharedMemoryMat(matricesptr);

                //calculate
                std::cout << "GPU execution time:"
                          << measure<>::execution([&controller, &matricesptr, &resultsptr]() {
                              controller.calculate(matricesptr, resultsptr);
                          })
                          << std::endl;
                std::cout << "CPU execution time:"
                          << measure<>::execution([]() { calculateCPU(); })
                          << std::endl;
                if (compare(resultsCPU, resultsptr, semples)) {
                    std::cout << "Results equal" << std::endl;
                } else {
                    std::cout << "ERROR! Results not equal" << std::endl;
                }

                printSharedMemoryRes(resultsptr);
                writeToSocket("finish");

            } else if (boost::starts_with(msg, "order")) {

                std::vector<std::string> results;
                boost::split(results, msg, [](char c){return c == ' ';});
                order = stoi(results[results.size() - 1]);
                std::cout << "change order to " << order << std::endl;
                controller.initOrder(order);

            } else if (boost::starts_with(msg, "samples")) {

                std::vector<std::string> results;
                boost::split(results, msg, [](char c){return c == ' ';});
                semples = stoi(results[results.size() - 1]);
                std::cout << "change samples to " << semples << std::endl;
                controller.initNumberSamples(semples);
                writeToSocket("number of samples changed");
            }
        }
    } else {
        testTimes();
    }
    return 0;
}

