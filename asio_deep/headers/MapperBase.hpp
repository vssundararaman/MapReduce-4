//
// Created by sakkammadam on 11/6/22.
//

#ifndef MRHOPE_MAPPERBASE_H
#define MRHOPE_MAPPERBASE_H

#include <string>
#include <tuple>
#include <vector>
#include <map>
#include <sstream>

class MapperBase{
// Private data members
private:
    int partitionNum;
    std::map<std::string, std::vector<std::string>> processedFilePartition;
    std::map<std::string, std::vector<std::vector<std::tuple<std::string, int, int>>>> mapperOutput;

public:
    // Default Constructor
    MapperBase(){
      // Nothing here
    };

    // Initialization Constructor
    explicit MapperBase(
            // originating partition num
            const int &partitionNum,
            // Get the processed directory file object
            const std::map<std::string, std::vector<std::string>> &processedFilePartition
    ){
        this->setPartitionNum(partitionNum);
        this->setProcessedFilePartition(processedFilePartition);
    }

    // Destructor
    virtual ~MapperBase(){};

    // Setters
    // This will set the processedDirectory private data member within the Mapper object
    void setProcessedFilePartition(const std::map<std::string, std::vector<std::string>> &processed_file_partition){
        this->processedFilePartition = processed_file_partition;
    }

    // This will set the partition num
    void setPartitionNum(const int partition_num){
        this->partitionNum = partition_num;
    }

    // Setters
    // This will store the processed map output as a private data member
    void setMapperOutputData(const std::map<std::string, std::vector<std::vector<std::tuple<std::string, int, int>>>> &mapped_output){
        this->mapperOutput = mapped_output;
    }

    // Getters
    // This will retrieve the processedDirectory private data member within the Mapper object
    std::map<std::string, std::vector<std::string>> getProcessedFilePartition(){
        return this->processedFilePartition;
    }

    // This will retrieve the partition num
    int getPartitionNum(){
        return this->partitionNum;
    }

    // This will retrieve the mapped output data private data member within the Mapper object
    std::map<std::string, std::vector<std::vector<std::tuple<std::string, int, int>>>> getMapperOutput(){
        return this->mapperOutput;
    }

    // Virtual method to run operations
    // Primary method that will act on processed input data and create a map
    virtual void runMapOperation() = 0;

};

// the types of the class factories
typedef MapperBase* createMapper_t(const int partitionNum, const std::map<std::string, std::vector<std::string>> &inputPartition);
typedef void destroyMapper_t(MapperBase*);


#endif //MRHOPE_MAPPERBASE_H
