#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <fstream>
#include <assert.h> 
#include <string>
#include <iomanip>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <sys/times.h>
#include <poll.h>
#include <vector>

#define MAXWORD

typedef struct { 
    int id; 
    char packet[10];
    char objectName[32]; 

} serverPacket;

typedef struct {
    int id;
    char objectName[32];

} storedInfo;

void sendToServer (int fifoCS, serverPacket cmd){
    write(fifoCS, (char *) &cmd, sizeof(serverPacket));
}

void do_client(std::string inputFile, int idNum){
    printf("main: do_client (idNumber= %d, inputFile= %s)\n\n", idNum, inputFile.c_str());
    int fifoSC;
    int fifoCS;
    std::ifstream infile(inputFile);
    if (!infile.is_open()) {
        std::cerr << "Failed to open input file: " << inputFile <<std::endl;
        exit(1);
    }

    if(idNum == 1){

        if( (fifoSC = open("fifo-0-1", O_RDWR)) == -1){ std::cout<<"Unable to open fifo-0-1."<<std::endl; };   
        if( (fifoCS = open("fifo-1-0", O_RDWR)) == -1){ std::cout<<"Unable to open fifo-1-0."<<std::endl; };   
            
    }
    else if(idNum == 2){
        
        if( (fifoSC = open("fifo-0-2", O_RDWR)) == -1){ std::cout<<"Unable to open fifo-0-2."<<std::endl; };   
        if( (fifoCS = open("fifo-2-0", O_RDWR)) == -1){ std::cout<<"Unable to open fifo-2-0."<<std::endl; };   
         
    }
    else if(idNum == 3){
        
        if( (fifoSC = open("fifo-0-3", O_RDWR)) == -1){ std::cout<<"Unable to open fifo-0-3."<<std::endl; };   
        if( (fifoCS = open("fifo-3-0", O_RDWR)) == -1){ std::cout<<"Unable to open fifo-3-0."<<std::endl; };   
        
    }

    std::string line;
    
    while(std::getline(infile, line) ){

        char *copy = strdup(line.c_str());

        char *token = strtok(copy, " ");

        if(token != NULL && token[0] == '#'){
            
            continue;
        }
        
        else if(line.empty() || line == "\n"){

            continue;
        }
        else{

            int inputIdNum = atoi(token);

            token = strtok(NULL, " ");
            std::string packet = token;
            token = strtok(NULL, " ");
            
            
            if(idNum != inputIdNum){    // skip if specified client is not current client
                continue;
            }
            else if(packet.compare("delay") == 0){
                std::string thirdArg = token;
                std::cout<<"*** Entering a delay period of "<<thirdArg<<" msec"<<std::endl;
                sleep((stoi(thirdArg))/1000);   // sleep for specified time
                std::cout<<"*** Exiting delay period"<<std::endl<<std::endl;
            }
            else if(packet.compare("quit") == 0){
                break;
            }
            else if(packet.compare("gtime") == 0){
                serverPacket cmd;
                memset((char *) &cmd, 0, sizeof(cmd));
                cmd.id = idNum;
                strcpy(cmd.packet, packet.c_str());
                sendToServer(fifoCS, cmd);
                std::cout<<"Transmitted (src= client:"<<idNum<<") GTIME"<<std::endl;
                
                char message[32];
                read(fifoSC, (char *) &message, sizeof(message));
                std::cout<<"Received (src= server) "<<message<<std::endl<<std::endl;
            }
            else{
                std::string thirdArg = token;
                serverPacket cmd;
                memset((char *) &cmd, 0, sizeof(cmd));
                cmd.id = idNum;
                strcpy(cmd.packet, packet.c_str());
                strcpy(cmd.objectName, thirdArg.c_str());
                sendToServer(fifoCS, cmd);
                std::cout<<"Transmitted (src= client:"<<idNum<<") ("<<boost::to_upper_copy(packet)<<": "<<thirdArg<<")"<<std::endl;
                char message[32];
                read(fifoSC, (char *) &message, sizeof(message));
                std::cout<<"Received (src= server) "<<message<<std::endl<<std::endl;
            }
        }
    }
    close(fifoSC);
    close(fifoCS);
    infile.close();
}

void do_server(){
    std::cout<<"a2p2: do_server."<<std::endl;
    std::vector<storedInfo> infoReceived;       // used to store client puts

    // start recording time
    long tics_per_second;
    tics_per_second = sysconf(_SC_CLK_TCK);
    struct tms tms_start;
    clock_t start;
    start = times(&tms_start);

    // Open fifos for server
    int fifo0_1;
    if( (fifo0_1 = open("fifo-0-1", O_RDWR)) == -1){ std::cout<<"Unable to open fifo-0-1."<<std::endl; };     
    int fifo1_0;
    if( (fifo1_0 = open("fifo-1-0", O_RDWR)) == -1){ std::cout<<"Unable to open fifo-1-0."<<std::endl; };   
    int fifo0_2;
    if( (fifo0_2 = open("fifo-0-2", O_RDWR)) == -1){ std::cout<<"Unable to open fifo-0-2."<<std::endl; };     
    int fifo2_0;
    if( (fifo2_0 = open("fifo-2-0", O_RDWR)) == -1){ std::cout<<"Unable to open fifo-2-0."<<std::endl; };   
    int fifo0_3;
    if( (fifo0_3 = open("fifo-0-3", O_RDWR)) == -1){ std::cout<<"Unable to open fifo-0-3."<<std::endl; };   
    int fifo3_0;
    if( (fifo3_0 = open("fifo-3-0", O_RDWR)) == -1){ std::cout<<"Unable to open fifo-3-0."<<std::endl; };   

    struct pollfd fds[4];


    // create pollfd array with client serve FIFOs
    fds[0].fd = fifo1_0; 
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = fifo2_0; 
    fds[1].events = POLLIN;
    fds[1].revents = 0;
    fds[2].fd = fifo3_0; 
    fds[2].events = POLLIN;
    fds[2].revents = 0;
    fds[3].fd = STDIN_FILENO;
    fds[3].events = POLLIN;
    fds[3].revents = 0;

    bool quit = false;
    int ret;
    
    while(!quit){
        ret = poll(fds, 4, 0);
        if(ret > 0){    // event has occured in something

            for(int i = 0; i < 3; i++){
                if(fds[i].revents & POLLIN){        //read from fifo
                    
                    fds[i].revents = 0;
                    serverPacket inPacket;
                    memset((char *) &inPacket, 0, sizeof(inPacket) );
                    read(fds[i].fd, (char *) &inPacket, sizeof(inPacket));
                    
                    
                    int currfifo;       // assign which client we have to send to
                    if(inPacket.id == 1){
                        currfifo = fifo0_1;
                    }
                    else if(inPacket.id == 2){
                        currfifo = fifo0_2;
                    }
                    else if(inPacket.id == 3){
                        currfifo = fifo0_3;
                    }
                    else{
                        std::cout<<"Client ID must be 1, 2, or 3."<<std::endl;
                    }

                    // Code for when packet is put
                    if(strcmp(inPacket.packet,"put") == 0){  // code for PUT packet
                        std::cout<<"Received (src= client "<<inPacket.id<<") (PUT: "<<(inPacket.objectName)<<")"<<std::endl;
                        bool exists = false;
                        for(int j = 0; j < (int)infoReceived.size(); j++){
                            if(strcmp(infoReceived[j].objectName,inPacket.objectName) == 0){  // check if object name already exists in server
                                exists = true;
                                break;
                            }
                        }
                        std::string message;
                        char msg2[32];
                        if(!exists){    // object name does not already exist
                            storedInfo temp;
                            memset((char *) &temp, 0, sizeof(temp));
                            temp.id = inPacket.id;
                            strcpy(temp.objectName, inPacket.objectName);
                            infoReceived.push_back(temp);
                            message = "OK";
                            strcpy(msg2, message.c_str());
                            write(currfifo, &msg2, sizeof(msg2));
                        }
                        else{
                            message = "(ERROR: object already exists)";
                            strcpy(msg2, message.c_str());
                            write(currfifo, &msg2, sizeof(msg2));
                        }
                        std::cout<<"Transmitted (src= server) "<<msg2<<std::endl<<std::endl;
                    }
                    else if(strcmp(inPacket.packet,"get") == 0){ // code for GET packet
                        std::cout<<"Received (src= client "<<inPacket.id<<") (GET: "<<(inPacket.objectName)<<")"<<std::endl;
                        bool exists = false;
                        for(int j = 0; j < (int)infoReceived.size(); j++){
                            if(strcmp(infoReceived[j].objectName,inPacket.objectName) == 0){  // check if object name already exists in server
                                exists = true;
                                break;
                            }
                        }

                        std::string message;
                        char msg2[32];
                        if(exists){     // get if it does exist
                            
                            message = "OK";
                            strcpy(msg2, message.c_str());
                            write(currfifo, &msg2, sizeof(msg2));
                        }
                        else{   // send error if file does not exist
                            message = "(ERROR: object not found)";
                            strcpy(msg2, message.c_str());
                            write(currfifo, &msg2, sizeof(msg2));
                        }
                        std::cout<<"Transmitted (src= server) "<<msg2<<std::endl<<std::endl;
                    }
                    else if(strcmp(inPacket.packet,"delete") == 0){  // code for delete packet
                        std::cout<<"Received (src= client "<<inPacket.id<<") (DELETE: "<<(inPacket.objectName)<<")"<<std::endl;
                        bool exists = false;
                        int pos;
                        for(int j = 0; j < (int)infoReceived.size(); j++){
                            if(strcmp(infoReceived[j].objectName,inPacket.objectName) == 0){  // check if object name already exists in server
                                exists = true;
                                pos = j;
                                break;
                            }
                        }

                        std::string message;
                        char msg2[32];
                        if(exists){     // delete if it does exist
                            if(infoReceived[pos].id == inPacket.id){
                                infoReceived.erase(infoReceived.begin() + pos);
                                message = "OK";
                                strcpy(msg2, message.c_str());
                                write(currfifo, &msg2, sizeof(msg2));
                            }
                            else{
                                message = "(ERROR: client not owner)";
                                strcpy(msg2, message.c_str());
                                write(currfifo, &msg2, sizeof(msg2));
                            }
                            
                        }
                        else{   // send error if file does not exist
                            message = "(ERROR: object not found)";
                            strcpy(msg2, message.c_str());
                            write(currfifo, &msg2, sizeof(msg2));
                        }
                        std::cout<<"Transmitted (src= server) "<<msg2<<std::endl<<std::endl;
                    }
                    else if(strcmp(inPacket.packet,"gtime") == 0){   // code for GTIME packet
                        std::cout<<"Received (src= client "<<inPacket.id<<") GTIME"<<std::endl;
                        char msg2[32];
                        struct tms tms_curr;
                        clock_t currTime = times(&tms_curr);
                        double timeNum = (currTime - start)/(double)tics_per_second;
                        
                        std::stringstream ss;
                        ss << std::fixed << std::setprecision(2) << timeNum;
                        std::string time = ss.str();
                        std::string message = "(TIME:   " + time + ")";
                        strcpy(msg2, message.c_str());

                        write(currfifo, &msg2, sizeof(msg2));
                        std::cout<<"Transmitted (src= server) "<<msg2<<std::endl<<std::endl;
                    }
                    else{
                        std::cout<<"Received (src= client "<<inPacket.id<<") "<<inPacket.packet<<std::endl;
                        std::string message = "(ERROR: invalid packet type)";
                        
                        char msg2[32];
                        strcpy(msg2, message.c_str());
                        write(currfifo, &msg2, sizeof(msg2));
                        std::cout<<"Transmitted (src= server) "<<msg2<<std::endl<<std::endl;
                    }
                }
            }
            if(fds[3].revents & POLLIN){
                fds[3].revents = 0;
                char userInput[5];
                read(fds[3].fd, (char *) &userInput, sizeof(userInput));

                userInput[strcspn(userInput, "\n")] = 0;

                if(strcmp(userInput,"list") == 0){
                    std::cout<<"Object table:"<<std::endl;
                    for(auto client : infoReceived){
                        std::cout<<"(owner= "<<client.id<<", name= "<<client.objectName<<")"<<std::endl;
                    }
                    std::cout<<std::endl;
                }
                else if(strcmp(userInput,"quit") == 0){
                    std::cout<<"quitting"<<std::endl;
                    quit = true;
                }
                else{
                    std::cout<<"Invalid user input. Must be either \'quit\' or \'list\'."<<std::endl;
                }
            }
        }
    }
    close(fifo0_1);
    close(fifo1_0);
    close(fifo0_2);
    close(fifo2_0);
    close(fifo0_3);
    close(fifo3_0);
}
int main(int argc, char *argv[]){
    if(argc == 1){      // error message if invalid arguments passed
        std::cout<<"Usage: "<<argv[0]<<" -s"<<"   Or     Usage: "<<argv[0]<<" -c idNumber inputFile";
        exit(0);
    }
    
    if(argc == 2 && strcmp(argv[1], "-s") == 0){    // run server if -s entered
        do_server();
    }
    else if(strcmp(argv[1], "-s") == 0){    // check input error
        std::cout<<"Usage: "<<argv[0]<<" -s"<<"   Or     Usage: "<<argv[0]<<" -c idNumber inputFile";
        exit(0);
    }
    else if(argc == 2){ // check input error
        std::cout<<"Usage: "<<argv[0]<<" -s"<<"   Or     Usage: "<<argv[0]<<" -c idNumber inputFile";
        exit(0);
    }
    
    if(argc == 4 && (strcmp(argv[1], "-c") == 0)){      // run client if -c entered
        int idNum = atoi(argv[2]);
        std::string inputFile = argv[3];
        do_client(inputFile, idNum);
    }
    else if(strcmp(argv[1], "-c") == 0){    // check input error
        std::cout<<"Usage: "<<argv[0]<<" -s"<<"   Or     Usage: "<<argv[0]<<" -c idNumber inputFile";
        exit(0);
    }
    else if(argc == 4){ // check input error
        std::cout<<"Usage: "<<argv[0]<<" -s"<<"   Or     Usage: "<<argv[0]<<" -c idNumber inputFile";
        exit(0);
    }

    return 0;
    
}