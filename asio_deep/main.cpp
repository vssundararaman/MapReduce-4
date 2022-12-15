#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <thread>
#include <future>
#include <mutex>
#include <filesystem>
#include <dlfcn.h>
#include <algorithm>
#include "headers/FileProcessorBase.hpp"
#include "headers/MapperBase.hpp"
#include "headers/ShufflerBase.hpp"
#include "headers/ReducerBase.hpp"

using namespace boost::asio;
using ip::tcp;

bool heartbeat_dispatch = true;

// global process message
const char* process_msg = "KEEP GOING!";

// global process counts
int count = 0;

// Mutex for mapper operations
std::mutex mapper_mutex;
std::mutex mapper_ind_mutex;
std::mutex mapper_op_mutex;
std::mutex mapper_ld_mutex;
std::mutex mapper_fp_mutex;

// Mutex for shuffler operations
std::mutex shuffler_mutex;
std::mutex shuffler_ind_mutex;
std::mutex shuffler_op_mutex;
std::mutex shuffler_ld_mutex;
std::mutex shuffler_fp_mutex;

// Mutex for reducer operations
std::mutex reducer_mutex;
std::mutex reducer_ind_mutex;
std::mutex reducer_op_mutex;
std::mutex reducer_ld_mutex;
std::mutex reducer_fp_mutex;

std::string standard_message(){
    std::string msg;
    int randomNumber = -100;
    if(strcmp(process_msg,"MAPPER COMPLETE!")==0){
        msg = "All mapper operations complete!\n";
    } else if(strcmp(process_msg,"SHUFFLER COMPLETE!")==0){
        msg = "All shuffler operations complete!\n";
    } else if(strcmp(process_msg,"REDUCER COMPLETE!")==0){
        msg = "EXIT!\n";
    } else {
        // invoke rand - note we perform %4 - means the random values will be in range of 0, 1, 2 and 3
        randomNumber = rand()%4;
        // leverage a switch to select
        switch (randomNumber) {
            case 0:
                msg = "Server Message -> Very Good!\n";
                break;
            case 1:
                msg = "Server Message -> Excellent!\n";
                break;
            case 2:
                msg = "Server Message -> Nice work!\n";
                break;
            case 3:
                msg = "Server Message -> Keep up the good work!\n";
                break;
            default:
                msg = "Server Message -> You are doing great!!\n";
        }
    }
    return msg;
}

// read from socket
std::string read_message(tcp::socket & socket) {
    boost::asio::streambuf buf;
    boost::asio::read_until( socket, buf, "\n" );
    std::string data = boost::asio::buffer_cast<const char*>(buf.data());
    return data;
}


// write to socket
void send_message(tcp::socket & socket, const std::string& message) {
    const std::string msg = message;
    boost::asio::write( socket, boost::asio::buffer(message) );
}

// library handle function
// This function will take a library file and return its corresponding handle as a null pointer
// https://linux.die.net/man/3/dlopen
void* createLibHandle(const char* libraryFile){
    // load library file
    void* libFileHandle = dlopen(libraryFile,RTLD_LAZY);
    // returning this so that it can be used createLibFunction template - used to create class instances
    // and can be used to close independently
    return libFileHandle;
}

// library operations - we will use a template here
// The goal of this template function is to return a function pointer corresponding
// to a factory function assigned to a particular library. It will take location of
// a library file and return the requested typedef for factory function
// We will then use the factory function to create an instance!
template<typename T>
T* createLibFunc(void* libHandle,const char* libraryFile,const char* factoryFunctionHandle){
    // check the type
    static_assert(
            std::is_same<T,create_t>::value ||
            std::is_same<T, destroy_t>::value ||
            std::is_same<T, createMapper_t>::value ||
            std::is_same<T, destroyMapper_t>::value ||
            std::is_same<T, readMapperOp_t>::value ||
            std::is_same<T, destroyMapperOp_t>::value ||
            std::is_same<T, createShuffler_t>::value ||
            std::is_same<T, destroyShuffler_t>::value ||
            std::is_same<T, readShufflerOp_t>::value ||
            std::is_same<T, destroyShufflerOp_t>::value ||
            std::is_same<T, createReducer_t>::value ||
            std::is_same<T, destroyReducer_t>::value ||
            std::is_same<T, readReducerOp_t>::value ||
            std::is_same<T, destroyReducerOp_t>::value,
            "Unsupported Implementation!"
    );
    // raise error if library wasn't loaded
    if(!libHandle){
        throw std::runtime_error("Cannot load library: " + std::string(dlerror()) + "\n");
    }
    // reset errors
    dlerror();
    // load the requested symbol associated with the function pointer which will
    // be used to create instance objects
    // https://linux.die.net/man/3/dlsym
    T* libFunc = (T*)dlsym(libHandle, factoryFunctionHandle);
    // capture any error
    const char* dlsym_error = dlerror();
    // check any error
    if(dlsym_error){
        throw std::runtime_error("Cannot load symbol from- " + std::string(libraryFile) + ":" + dlsym_error + "\n");
    }
    // else return
    return libFunc;
}

// Function that will take FileProcessorBase (overloaded against FileProcessorInput via polymorphism)
// Creates a memory object against a single file -> map(fileName, vector of vectors) - each inner vector contains
// data belonging to a "partition" - ~ 2k records
auto fileProcessInputs(FileProcessorBase* obj){
    obj->runOperation();
    return obj->getInputDirectoryData();
}

// Function that will take MapperBase (overloaded against MapperImpl via polymorphism)
// Creates a mapper object against a PARTITION of a file memory object
// Produces a mapper dataset in memory that contains a map of tuples
auto mapperOps(MapperBase* obj){
    mapper_ind_mutex.lock();
    obj->runMapOperation();
    mapper_ind_mutex.unlock();
    return obj->getMapperOutput();
}

// Function that will take FileProcessorBase (overloaded against FileProcessorMapOutput via polymorphism)
// Takes mapper memory data structure and persists to disk
auto fileProcessMapOutputs(FileProcessorBase* obj){
    mapper_fp_mutex.lock();
    obj->runOperation();
    mapper_fp_mutex.unlock();
    return obj->getMapperOutputDirectory();
}

// Function that will take ShufflerBase (overloaded against ShufflerImpl via polymorphism)
// Creates a shuffler object against a temp_mapper subdirectory
// Produces a shuffler dataset in memory that contains a map of tuples, the value being an aggregated of all keys
auto shufflerOps(ShufflerBase* obj){
    shuffler_ind_mutex.lock();
    obj->runShuffleOperation();
    shuffler_ind_mutex.unlock();
    return obj->getShuffledOutput();
}

// Function that will take FileProcessorBase (overloaded against FileProcessorShufOutput via polymorphism)
// Takes shuffler memory data structure and persists to disk
auto fileProcessShufOutputs(FileProcessorBase* obj){
    shuffler_fp_mutex.lock();
    obj->runOperation();
    shuffler_fp_mutex.unlock();
    return obj->getShufflerOutputDirectory();
}

// Function that will take ReducerBase (overloaded against ReducerImpl via polymorphism)
// Creates a reducer object against a temp_shuffler subdirectory
// Produces a reducer dataset in memory that contains a map of tuples, the value being an aggregated of all keys, across all shuffler files
auto reducerOps(ReducerBase* obj){
    reducer_ind_mutex.lock();
    obj->runReduceOperations();
    reducer_ind_mutex.unlock();
    return obj->getReducedOutput();
}

// Function that will take FileProcessorBase (overloaded against FileProcessorRedOutput via polymorphism)
// Takes reducer memory data structure and persists to disk - this is the final output
auto fileProcessRedOutputs(FileProcessorBase* obj){
    reducer_fp_mutex.lock();
    obj->runOperation();
    reducer_fp_mutex.unlock();
    return obj->getFinalOutputDirectory();
}

// Mapper operations
std::string mapperOperation(const std::string &input_directory){
    // Load a handle corresponding to FileProcessorInput library
    void* fpInputLibHandle = createLibHandle("./libs/fp/FileProcessorInput.so");
    // Load the FileProcessorInput library!
    // load the symbols associated with function pointer that will create new base instance object of FileProcessorInput
    // it will call the constructor specifically for input operations!
    create_t* create_InputDirectoryFP_Obj = createLibFunc<create_t>(
            fpInputLibHandle,
            "./libs/fp/FileProcessorInput.so",
            "createInputObj");
    // declare a vector that will hold the all files in a directory!
    std::vector<std::string> directory_files;
    // iterate and load directory_files vector!
    for(const auto &entry:std::filesystem::directory_iterator(input_directory)){
        if(std::filesystem::is_regular_file(entry)){
            directory_files.push_back(entry.path());
        }
    }
    // declare a vector that will hold all fileProcessorInput objects
    std::vector<FileProcessorBase*> fp_objects;
    // use directory_files vector to load fp_objects vector
    for(const auto &file: directory_files){
        fp_objects.push_back(create_InputDirectoryFP_Obj("input",file));
    }
    // declare a vector of futures that will host results of file processor input operations
    std::vector<std::future<std::map<std::string, std::vector<std::vector<std::string>>>>> load_dir_files;
    // use fp_objects vector to call individual objects and load the load_dir_files vector
    for(auto obj: fp_objects){
        load_dir_files.push_back(std::async(fileProcessInputs,obj));
    }
    std::cout << "There are " << load_dir_files.size() << " future objects in load_dir_files..." << std::endl;

    // Load a handle corresponding to Mapper library
    void* mapLibHandle = createLibHandle("./libs/map/MapperImpl.so");
    // Load the Mapper library!
    // load the symbols associated with function pointer that will create new instance object of MapperBase
    createMapper_t* create_Mapper_Obj = createLibFunc<createMapper_t>(
            mapLibHandle,
            "./libs/map/MapperImpl.so",
            "createInputObj");

    // declare a vector that will hold all mapper objects
    std::vector<MapperBase*> mapper_objects;

    // iterate over the load_dir_files vector...
    for(auto i=0; i < load_dir_files.size(); i++){
        const std::map<std::string, std::vector<std::vector<std::string>>> &retInput = load_dir_files[i].get();
        // displaying data!
        for(const auto& row: retInput){
            std::cout << row.first << std::endl;
            for(int _i=0; _i < row.second.size(); _i++){
                std::cout << "Operating on file - " << row.first << std::endl;
                std::cout << "Working on partition#" << _i << std::endl;
                // create temp obj
                std::map<std::string, std::vector<std::string>> tempObj;
                tempObj.insert({row.first,row.second[_i]});
                std::cout << "Creating Mapper#" << _i << std::endl;
                mapper_objects.push_back(create_Mapper_Obj(_i, tempObj));
            }
        }
    }
    // declare a vector of futures that will host results of mapper operations
    std::vector<std::future<std::map<std::string, std::vector<std::vector<std::tuple<std::string, int, int>>>>>> mapped_data;

    // use mapper_objects vector to call individual objects and load the mapper_data vector
    std::cout << "There are " << mapper_objects.size() << " mappers" << std::endl;

    // run the mapper operations using mappers!
    for(auto obj:mapper_objects){
        mapper_mutex.lock();
        mapped_data.push_back(std::async(mapperOps,obj));
        mapper_mutex.unlock();
    }

    std::cout << "There are " << mapped_data.size() << " future objects in mapper_data vector...." << std::endl;

    // Load a handle corresponding to FileProcessorMapOutput library
    void* fpMapOpLibHandle = createLibHandle("./libs/fp/FileProcessorMapOutput.so");
    // Load the FileProcessorMapOutput library!
    readMapperOp_t* create_MapperFP_Obj = createLibFunc<readMapperOp_t>(
            fpMapOpLibHandle,
            "./libs/fp/FileProcessorMapOutput.so",
            "createInputObj");

    // declare a vector that will hold all fileProcessorMapOutput objects
    std::vector<FileProcessorBase*> fp_map_outputs;

    // load the mapper output to disk using FileProcessorMapOutput
    for(auto i=0; i < mapped_data.size(); i++){
        mapper_op_mutex.lock();
        const std::map<std::string, std::vector<std::vector<std::tuple<std::string, int, int>>>> &retInput = mapped_data[i].get();
        // supply retInput as arguments to FileProcessorMapOutput
        fp_map_outputs.push_back(create_MapperFP_Obj("mapper",retInput));
        mapper_op_mutex.unlock();
    }

    // declare a vector of futures that will host results of mapper file processor output operations
    std::vector<std::future<std::string>> fp_map_output_dirs;
    std::mutex hope; // this could be removed!
    hope.lock(); // this could be removed!
    // run the file processor mapper output operation...
    for(auto obj: fp_map_outputs){
        mapper_ld_mutex.lock();
        fp_map_output_dirs.push_back(std::async(fileProcessMapOutputs,obj));
        mapper_ld_mutex.unlock();
    }
    hope.unlock(); // this could be removed!
    std::string mapper_root_directory;

    // this is to make sure all the mapper operations complete!
    for(auto &fut_map: fp_map_output_dirs){
        mapper_root_directory = fut_map.get();
    }
    // Eventually it will finish...
    std::cout << "All mapper output has been written to this directory - " << mapper_root_directory << std::endl;

    // set global value
    process_msg = "MAPPER COMPLETE!";

    // increase global count
    count++;

    // return
    return mapper_root_directory;

}

// Shuffler operations
std::string shufflerOperations(const std::string &mapper_root_directory){
    // declare a vector that will hold the all mapper folders!
    std::vector<std::string> mapper_folders;
    // iterate and load directory_files vector!
    for(const auto &entry:std::filesystem::directory_iterator(mapper_root_directory)){
        mapper_folders.push_back(entry.path());
    }
    std::cout << "The individual temp_mapper folders are: " << std::endl;
    for(const std::string &folder:mapper_folders){
        std::cout << folder << std::endl;
    }
    std::cout << "Proceeding to create Shuffler objects to operate against temp_mapper sub-folders..." << std::endl;

    // Load a handle corresponding to Shuffler library
    void* shufLibHandle = createLibHandle("./libs/shuffle/ShufflerImpl.so");
    // Load the Shuffler library!
    createShuffler_t* create_Shuffler_Obj = createLibFunc<createShuffler_t>(
            shufLibHandle,
            "./libs/shuffle/ShufflerImpl.so",
            "createInputObj");
    // declare a vector of shuffler objects
    std::vector<ShufflerBase*> shuffler_objects;
    // load the vector of shuffler objects by supplying the individual temp_mapper folders...
    for(const std::string &folder:mapper_folders){
        shuffler_objects.push_back(create_Shuffler_Obj(folder));
    }

    // declare a vector to store future results of shuffler operations
    std::vector<std::future<std::vector<std::map<std::string, std::map<std::string,size_t>>>>> shuffler_data;
    // load the vector with shuffler futures
    for(auto obj:shuffler_objects){
        shuffler_mutex.lock();
        shuffler_data.push_back(std::async(shufflerOps,obj));
        shuffler_mutex.unlock();
    }
    std::cout << "There are " << shuffler_data.size() << " future objects in shuffler_data vector...." << std::endl;

    // Load a handle corresponding to FileProcessorShufOutput library
    void* fpShufOpLibHandle = createLibHandle("./libs/fp/FileProcessorShufOutput.so");
    // Load the FileProcessorShufOutput library
    readShufflerOp_t* create_ShufflerFP_Obj = createLibFunc<readShufflerOp_t>(
            fpShufOpLibHandle,
            "./libs/fp/FileProcessorShufOutput.so",
            "createInputObj");

    // declare a vector that will hold all fileProcessorShufOutput objects
    std::vector<FileProcessorBase*> fp_shuf_outputs;
    // pass the shuffler output to FileProcessorShufOutput
    for(auto i=0; i < shuffler_data.size(); i++){
        shuffler_op_mutex.lock();
        const std::vector<std::map<std::string, std::map<std::string,size_t>>> &shufOutput = shuffler_data[i].get();
        shuffler_op_mutex.unlock();
        // supply shufOutput as arguments to FileProcessorShufOutput
        fp_shuf_outputs.push_back(create_ShufflerFP_Obj("shuffler",shufOutput));
    }
    // declare a vector of futures that will host results of shuffler file processor output operations
    std::vector<std::future<std::string>> fp_shuf_output_dirs;
    // run the file processor shuffler output operation....
    // fileProcessShufOutputs will cause the obj to write data to disk and return the directory to fp_shuf_output_dirs
    for(auto obj: fp_shuf_outputs){
        shuffler_ld_mutex.lock();
        fp_shuf_output_dirs.push_back(std::async(fileProcessShufOutputs,obj));
        shuffler_ld_mutex.unlock();
    }

    // initialize
    std::string shuffler_root_directory;

    //shuffler root directory
    // this is to make sure all the shuffler operations complete!
    for(auto &fut_shuf: fp_shuf_output_dirs){
        shuffler_root_directory = fut_shuf.get();
    }

    // Eventually it will finish...
    std::cout << "All shuffler output has been written to this directory - " << shuffler_root_directory << std::endl;

    // set global value
    process_msg = "SHUFFLER COMPLETE!";

    // increase global count
    count++;

    // return
    return shuffler_root_directory;

}

// Reducer operations
std::string reducerOperations(const std::string &shuffler_root_directory){
    // declare a vector that will hold the all shuffler folders!
    std::vector<std::string> shuffler_folders;
    // iterate and load directory_files vector!
    for(const auto &entry:std::filesystem::directory_iterator(shuffler_root_directory)){
        shuffler_folders.push_back(entry.path());
    }
    std::cout << "The individual temp_shuffler folders are: " << std::endl;
    for(const std::string &folder:shuffler_folders){
        std::cout << folder << std::endl;
    }

    // Load a handle corresponding to Reducer library
    void* redLibHandle = createLibHandle("./libs/reduce/ReducerImpl.so");
    // load the Reducer library
    createReducer_t* create_Reducer_Obj = createLibFunc<createReducer_t>(
            redLibHandle,
            "./libs/reduce/ReducerImpl.so",
            "createInputObj");
    std::cout << "Proceeding to create Reducer objects to operate against temp_shuffler sub-folders..." << std::endl;
    // declare a vector of shuffler objects
    std::vector<ReducerBase*> reducer_objects;
    // load the vector of reducer objects by supplying the individual temp_shuffler folders...
    for(const std::string &folder:shuffler_folders){
        reducer_objects.push_back(create_Reducer_Obj(folder));
    }
    // declare a vector to store future results of reducer operations
    std::vector<std::future<std::map<std::string, std::map<std::string,size_t>>>> reducer_data;
    // load the vector with reducer futures
    for(auto obj:reducer_objects){
        reducer_mutex.lock();
        reducer_data.push_back(std::async(reducerOps,obj));
        reducer_mutex.unlock();
    }
    std::cout << "There are " << reducer_data.size() << " future objects in reducer vector...." << std::endl;

    // Load a handle corresponding to FileProcessorRedOutput library
    void* fpRedOpLibHandle = createLibHandle("./libs/fp/FileProcessorRedOutput.so");
    // load the FileProcessorRedOutput library
    readReducerOp_t* create_ReducerFP_Obj = createLibFunc<readReducerOp_t>(
            fpRedOpLibHandle,
            "./libs/fp/FileProcessorRedOutput.so",
            "createInputObj");
    // declare a vector that will hold all fileProcessorRedOutput objects
    std::vector<FileProcessorBase*> fp_red_outputs;
    // pass the reducer output to FileProcessorRedOutput
    for(auto i=0; i < reducer_data.size(); i++){
        reducer_op_mutex.lock();
        const std::map<std::string, std::map<std::string,size_t>> &redOutput = reducer_data[i].get();
        // supply redOutput as arguments to FileProcessorRedOutput
        fp_red_outputs.push_back(create_ReducerFP_Obj("reducer",redOutput));
        reducer_op_mutex.unlock();
    }
    // declare a vector of futures that will host results of reducer file processor output operations
    std::vector<std::future<std::string>> fp_red_output_dirs;

    // run the file processor reducer output operation....
    // fileProcessRedOutputs will cause the obj to write data to disk and return the directory to fp_red_output_dirs
    for(auto obj: fp_red_outputs){
        reducer_ld_mutex.lock();
        fp_red_output_dirs.push_back(std::async(fileProcessRedOutputs,obj));
        reducer_ld_mutex.unlock();
    }

    // declare
    std::string reducerDir;
    // this is to make sure all the reducer operations complete!
    for(auto &fut_red: fp_red_output_dirs){
        reducerDir = fut_red.get();
    }

    // Eventually it will finish...
    std::cout << "All final output has been written to this root directory - " << reducerDir << std::endl;

    // set global value
    process_msg = "REDUCER COMPLETE!";

    // increase global count
    count++;

    // return
    return reducerDir;
}

int main() {
    boost::asio::io_service io_service;
    tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), 3333 ));
    acceptor_.set_option(tcp::acceptor::reuse_address(true));

    while(heartbeat_dispatch){
        tcp::socket socket_(io_service);
        acceptor_.accept(socket_);
        std::string client_message = read_message(socket_);
        std::cout << client_message << std::endl;

        std::cout << process_msg << std::endl;

        // standard
        std::string server_message = standard_message();
        send_message(socket_, server_message);
        std::cout << "Server sent message: " << server_message  << std::endl;

        // declare variables
        std::string mapper_results;
        std::string shuffle_results;
        std::string reduce_results;

        // Mapper operations!
        while(strcmp(process_msg,"MAPPER COMPLETE!")!=0 && count== 0){
            mapper_results = mapperOperation("/home/ubuntu/CLionProjects/shakespeare_2");
            std::string mapper_msg = "Mapper results written to: " + mapper_results + "\n";
            send_message(socket_, mapper_msg);
        }

        // Shuffle operations!
        while(strcmp(process_msg,"SHUFFLER COMPLETE!")!=0 && count== 1){
            shuffle_results = shufflerOperations(mapper_results);
            std::string shuffler_msg = "Shuffler results written to: " + shuffle_results + "\n";
            send_message(socket_, shuffler_msg);
        }

        // Reduce operations!
        while(strcmp(process_msg,"REDUCER COMPLETE!")!=0 && count== 2){
            reduce_results = reducerOperations(shuffle_results);
            std::string reducer_msg = "Reducer results written to: " + reduce_results + "\n";
            send_message(socket_, reducer_msg);
        }

    }
    io_service.run();


    return 0;
}