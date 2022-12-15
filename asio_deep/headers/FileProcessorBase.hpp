/*
 * Author: Vidyasankar Sundararaman
 * Description: FileProcessor Base (Abstract) implementation
 */
#ifndef MAPREDUCELIB_FILEPROCESSORBASE_HPP
#define MAPREDUCELIB_FILEPROCESSORBASE_HPP

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <tuple>
#include <fstream>
#include <filesystem>
#include <stdexcept>

class FileProcessorBase{
private:
    // directory operation - input for FileProcessInput implementation
    std::string directoryOperation;
    // directory path - input for FileProcessInput implementation
    std::string directoryPath;
    // input directory data - output by FileProcessInput implementation
    std::map<std::string, std::vector<std::vector<std::string>>> inputDirectoryData;
    // input Mapper data - output of Mapper implementation
    std::map<std::string, std::vector<std::vector<std::tuple<std::string, int, int>>>> mapperRawOutput;
    // output directory path that will be used to store the results of mapper operation
    std::string mapperOutputDirectory;
    // input Shuffler data - output of Shuffler implementation
    std::vector<std::map<std::string, std::map<std::string,size_t>>> shufflerRawOutput;
    // output directory path that will be used to store the results of shuffle operation
    std::string shufflerOutputDirectory;
    // input Reducer data - output of Reducer implementation
    std::map<std::string, std::map<std::string, size_t>> reducerRawOutput;
    // output directory path that will be used to store the final results
    std::string finalOutputDirectory;

public:
    // Constructor
    FileProcessorBase(const std::string &operation, const std::string &path){
        // Set Operation - will raise error if the value is incorrect
        this->setOperation(operation);
        // Set Directory path - will raise error if the path is not present in the file system
        this->setPath(path);
    }

    FileProcessorBase() {};

    // Destructor
    virtual ~FileProcessorBase(){};

    // Setter - operation
    void setOperation(const std::string &operation){
        if(operation == "input" || operation == "mapper" || operation == "shuffler" || operation == "reducer"){
            this->directoryOperation = operation;
        } else{
            throw std::runtime_error("Unsupported operation!: " + operation );
        }
    }
    // Getter - operation
    std::string getOperation(){
        return this->directoryOperation;
    }
    // Setter - directory path
    void setPath(const std::string &path){
        std::filesystem::path p(path);
        if(std::filesystem::exists(p)){
            this->directoryPath = path;
        } else {
            throw std::runtime_error("Directory not found!: " + path );
        }
    }
    // Getter - directory path
    std::string getDirectoryPath(){
        return this->directoryPath;
    }
    // Setter - all Input directory data
    void setInputDirectoryData(const std::map<std::string, std::vector<std::vector<std::string>>> &inputData){
        this->inputDirectoryData = inputData;
    }
    // Getter - all Input directory data
    std::map<std::string, std::vector<std::vector<std::string>>> getInputDirectoryData(){
        return this->inputDirectoryData;
    }
    // Setter - set the raw output from the Mapper
    void setRawMapperOutput(const std::map<std::string, std::vector<std::vector<std::tuple<std::string, int, int>>>> &raw_mapper_data){
        this->mapperRawOutput = raw_mapper_data;
    }
    // Getter - get the raw mapper output data
    std::map<std::string, std::vector<std::vector<std::tuple<std::string, int, int>>>> getRawMapperOutput(){
        return this->mapperRawOutput;
    }

    // Setter - mapper output directory path
    void setMapperOutputDirectory(const std::string &mapper_output_directory){
        this->mapperOutputDirectory = mapper_output_directory;
    }
    // Getter - mapper output directory path
    std::string getMapperOutputDirectory(){
        return this->mapperOutputDirectory;
    }

    // Setter - set the raw output from the Shuffler
    void setRawShufflerOutput(const std::vector<std::map<std::string, std::map<std::string,size_t>>> &raw_shuffler_data){
        this->shufflerRawOutput = raw_shuffler_data;
    }
    // Getter - get the raw shuffler output
    std::vector<std::map<std::string, std::map<std::string,size_t>>> getRawShufflerOutput(){
        return this->shufflerRawOutput;
    }

    // Setter - shuffler output directory path
    void setShufflerOutputDirectory(const std::string &shuffler_output_directory){
        this->shufflerOutputDirectory = shuffler_output_directory;
    }
    // Getter - shuffler output directory path
    std::string getShufflerOutputDirectory(){
        return this->shufflerOutputDirectory;
    }
    // Setter - set the raw output from the Reducer
    void setRawReducerOutput(const std::map<std::string, std::map<std::string, size_t>> &raw_reducer_data){
        this->reducerRawOutput = raw_reducer_data;
    }
    // Getter - get the raw reducer output
    std::map<std::string, std::map<std::string, size_t>> getRawReducerOutput(){
        return this->reducerRawOutput;
    }
    // Setter - set the final output directory
    void setFinalOutputDirectory(const std::string &directory){
        this->finalOutputDirectory = directory;
    }
    // Getter - get the final output directory
    std::string getFinalOutputDirectory(){
        return this->finalOutputDirectory;
    }

    // Helper
    void createDirectory(const std::string &directory_path) {
        if (std::filesystem::exists(directory_path)) {
            std::cout << "The " << this->getOperation() <<  " directory: " << directory_path << " already exists! " << std::endl;
        } else {
            // create directory!
            std::filesystem::create_directories(directory_path);
            std::cout << "Created the " << this->getOperation() << " directory: " << directory_path << std::endl;
        }
    }

    // Virtual method to run operations
    // Primary method that will be based on input, do necessary operations and store necessary data as private data members
    virtual void runOperation() = 0;

};

// Types of class factories:

// Read Input directory!
// Signifies the Factory that will read input directories using createInputObj within FileProcessorInput.cpp implementation
typedef FileProcessorBase* create_t(const std::string &operation, const std::string &path);
// Signify the Factory that will remove the instance within FileProcessInput.cpp implementation
typedef void destroy_t(FileProcessorBase*);

// Read Mapper outputs!
// Signifies the Factory that will read Mapper outputs using createInputObj within FileProcessMapOutput.cpp implementation
typedef FileProcessorBase* readMapperOp_t(
        const std::string &operation,
        const std::map<std::string, std::vector<std::vector<std::tuple<std::string, int, int>>>> &map_raw);
// Signify the Factory that will remove the instance within FileProcessMapOutput.cpp implementation
typedef void destroyMapperOp_t(FileProcessorBase*);

// Read Shuffler outputs!
// Signifies the Factory that will read Shuffler outputs using createInputObj within FileProcessorShufOutput.cpp implementation
typedef FileProcessorBase* readShufflerOp_t(
        const std::string &operation,
        const std::vector<std::map<std::string, std::map<std::string,size_t>>> &shuffle_raw);
// Signify the Factory that will remove the instance within FileProcessorShufOutput.cpp implementation
typedef FileProcessorBase*  destroyShufflerOp_t(FileProcessorBase*);

// Read Reducer outputs!
// Signifies the Factory that will read Reducer outputs using createInputObj within FileProcessorRedOutput.cpp implementation
typedef FileProcessorBase* readReducerOp_t(
        const std::string &operation,
        const std::map<std::string, std::map<std::string,size_t>> &reduced_raw);
// Signify the Factory that will remove the instance within FileProcessorRedOutput.cpp implementation
typedef FileProcessorBase*  destroyReducerOp_t(FileProcessorBase*);

#endif //MAPREDUCELIB_FILEPROCESSORBASE_HPP
