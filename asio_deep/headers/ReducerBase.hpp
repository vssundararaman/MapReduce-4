//
// Created by sakkammadam on 11/6/22.
//

#ifndef MRHOPE_REDUCERBASE_H
#define MRHOPE_REDUCERBASE_H
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <map>
#include <vector>

class ReducerBase{
private:
    // Private data member - directory containing shuffler outputs
    std::string parentShuffleDirectory;
    // Reduced output - contains processed partition files from the shuffle directory - in memory
    std::map<std::string, std::map<std::string, size_t>> reducedOutput;

public:
    // Default constructor
    ReducerBase(){};

    // Explicit Constructor that will take the shuffle directory as input
    explicit ReducerBase(const std::string &parent_shuffle_directory){
        // Set the private data member mapShuffleDirectory here
        this->setShuffleOutputDirectory(parent_shuffle_directory);
    }

    // Destructor
    virtual ~ReducerBase(){};

    // Setters
    // This method sets the parentShuffleDirectory private data member
    void setShuffleOutputDirectory(const std::string &parent_shuffle_directory){
        // Check if the shuffle directory exists on the file system
        std::filesystem::path directoryPath(parent_shuffle_directory);
        if(std::filesystem::is_directory(directoryPath)){
            this->parentShuffleDirectory = parent_shuffle_directory;
        } else {
            throw std::runtime_error("Directory not found!: " + parent_shuffle_directory );
        }
    }

    // Getters
    // This method retrieves the parentShuffleDirectory private data member
    std::string getShuffleOutputDirectory(){
        return this->parentShuffleDirectory;
    }

    // Setters
    // This method sets reducedOutput private data member
    void setReducedOutput(const std::map<std::string, std::map<std::string, size_t>> &reduced_result){
        this->reducedOutput = reduced_result;
    }

    // Getters
    // This method retrieves the reducedOutput private data member
    std::map<std::string, std::map<std::string, size_t>> getReducedOutput(){
        return this->reducedOutput;
    }

    // Virtual method to run reduce operations
    // Primary method that will act on shuffled files and create reduced results in memory
    virtual void runReduceOperations() = 0;

};

// the types of the class factories
typedef ReducerBase* createReducer_t(const std::string &shuffle_directory);
typedef void destroyReducer_t(ReducerBase*);

#endif //MRHOPE_REDUCERBASE_H
