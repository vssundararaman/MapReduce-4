#include <iostream>
#include <boost/asio.hpp>
#include <stdlib.h>

using namespace boost::asio;
using ip::tcp;

// global count
int count = 0;

std::string standard_message(){
    std::string msg;
    if(count == 0){
        msg = "Start Mapper Operation\n";
    } else if(count == 1){
        msg = "Start Shuffler Operation\n";
    } else if(count == 2){
        msg = "Start Reducer Operation\n";
    } else {
        int randomNumber = -100;
        // invoke rand - note we perform %4 - means the random values will be in range of 0, 1, 2 and 3
        randomNumber = rand()%4;
        // leverage a switch to select
        switch (randomNumber) {
            case 0:
                msg = " Message From Client -> Very Good!\n";
                break;
            case 1:
                msg = "Message From Client ->Excellent!\n";
                break;
            case 2:
                msg = "Message From Client ->Nice work!\n";
                break;
            case 3:
                msg = "Message From Client ->eep up the good work!\n";
                break;
            default:
                msg = "Message From Client ->You are doing great!!\n";
        }
    }
    return msg;
}

int main() {
    boost::asio::io_service io_service;
    tcp::socket socket(io_service);

    const char* terminate_msg = "START!";

    while(strcmp(terminate_msg,"EXIT!")!=0){
        // send data
        socket.connect(tcp::endpoint( boost::asio::ip::address::from_string("127.0.0.1"), 3333));
        const std::string client_message = standard_message();
        boost::system::error_code error;
        boost::asio::write( socket, boost::asio::buffer(client_message), error );
        // standard error tracking
        if(!error) {
            std::cout << "Client -> message: " << client_message << std::endl;
        }
        else {
            std::cout << "Client - > sent failure: " << error.message() << std::endl;
        }
        // receive data
        boost::asio::streambuf receive_buffer;
        boost::asio::read(socket, receive_buffer, boost::asio::transfer_all(), error);
        if( error && error != boost::asio::error::eof ) {
            std::cout << "Client -> message failed: " << error.message() << std::endl;
        }
        else {
            const char* data = boost::asio::buffer_cast<const char*>(receive_buffer.data());
            std::cout << data << std::endl;
            if(strcmp(data,"EXIT!\n")==0){
                terminate_msg = "EXIT!";
            }
        }
        socket.close();
    }
    return 0;
}