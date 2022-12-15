//
// Created by sakkammadam on 11/6/22.
//

#ifndef MRHOPE_SHUFFLERBASE_HPP
#define MRHOPE_SHUFFLERBASE_HPP

#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <map>
#include <fstream>

class ShufflerBase {
    //private data members
private:
    // directory name containing mapper outputs
    std::string mapOutputDirectory;
    // shuffled output
    std::vector<std::map<std::string, std::map<std::string,size_t>>> shuffledOutput;
    // public methods
public:
    // default Shuffler constructor
    ShufflerBase(){
        // we are not doing anything
    }
    // explicit constructor - reads a directory containing results of map operations
    explicit ShufflerBase(const std::string &mapper_directory){
        // sets the mapper directory private data member on constructor initialization
        setMapOutputDirectory(mapper_directory);
    }
    // destructor
    virtual ~ShufflerBase(){};
    // Setters -
    // This method sets the mapOutputDirectory private data member
    void setMapOutputDirectory(const std::string &mapper_directory){
        // Check if the mapper directory exists on the file system
        std::filesystem::path p(mapper_directory);
        if(std::filesystem::exists(p)){
            this->mapOutputDirectory = mapper_directory;
        } else {
            throw std::runtime_error("Directory not found!: " + mapper_directory );
        }
    }
    // This method sets the shuffledOutput private data member
    void setShuffledOutput(const std::vector<std::map<std::string, std::map<std::string,size_t>>> &shuffled_output){
        this->shuffledOutput = shuffled_output;
    }

    // Getters -
    // This method retrieves the mapOutputDirectory private data member
    std::string getMapOutputDirectory(){
        return this->mapOutputDirectory;
    }
    // This method will retrieve the shuffledOutput private data member
    std::vector<std::map<std::string, std::map<std::string,size_t>>> getShuffledOutput(){
        return this->shuffledOutput;
    }

    // Virtual method to run operations
    // Primary method that will act on processed mapped files and create shuffled results in memory
    virtual void runShuffleOperation() = 0;

};

// the types of the class factories
typedef ShufflerBase* createShuffler_t(const std::string &mapper_directory);
typedef void destroyShuffler_t(ShufflerBase*);


#endif //MRHOPE_SHUFFLERBASE_HPP
