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

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#define NCLIENT 3
#define MAXCHARS 80
#define MAXLINES 3
#define MAXWORD 32

int fd; // shared memory for client and server

typedef struct { 
    int id; 
    char packet[10];    // longest packet name is delete
    char objectName[MAXWORD + 1];
    char line1[MAXCHARS + 10];
    char line2[MAXCHARS + 10];
    char line3[MAXCHARS + 10];

} serverPacket;

typedef struct {
    int id;
    char objectName[MAXWORD + 1];
    char line1[MAXCHARS + 10];
    char line2[MAXCHARS + 10];
    char line3[MAXCHARS + 10];

} storedInfo;

typedef struct sockaddr SA;

void sendToServer (int fifoCS, serverPacket cmd){
    write(fifoCS, (char *) &cmd, sizeof(serverPacket));
}

int connectClient(const char* serverName, int portNum){
    int sfd;
    struct sockaddr_in server;
    struct hostent *hp;

    hp = gethostbyname(serverName);
    if(hp == (struct hostent *) NULL){
        std::cerr<<"gethostbyname failed."<<std::endl;
        exit(1);
    }
    
    memset((char *) &server, 0, sizeof(server));
    memcpy((char *)&server.sin_addr, hp->h_addr, hp->h_length);
    server.sin_family = AF_INET;
    server.sin_port = htons(portNum);

    if((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        std::cerr<<"Socket creation failed."<<std::endl;
        exit(1);
    }
    if(connect(sfd, (SA *) &server, sizeof(server)) < 0){
        std::cerr<<"Failed to connect socket."<<std::endl;
        exit(1);
    }



    return sfd;
}

int serverListen (int portNo, int nClient)
{
    int                 sfd;
    struct sockaddr_in  sin;

    memset ((char *) &sin, 0, sizeof(sin));

    // create a managing socket
    if ( (sfd= socket (AF_INET, SOCK_STREAM, 0)) < 0){
        std::cerr<<"serverListen: Failed to create socket."<<std::endl;
        exit(1);
    }


    // bind the managing socket to a name
    sin.sin_family= AF_INET;
    sin.sin_addr.s_addr= INADDR_ANY;
    sin.sin_port= htons(portNo);

    

    if (bind (sfd, (SA *) &sin, sizeof(sin)) < 0){
        std::cerr<<"serverListen: bind failed."<<std::endl;
        exit(1);
    }
    // indicate how many connection requests can be queued

    listen (sfd, nClient);
    return sfd;
}


void do_client(std::string inputFile, int idNum, char *serverName, int portNum){

    long tics_per_second;
    tics_per_second = sysconf(_SC_CLK_TCK);
    struct tms tms_start, tms_end;
    clock_t start, end;
    start = times(&tms_start);

    printf("main: do_client (idNumber= %d, inputFile= '%s') (server= '%s', port= %d)\n\n", idNum, inputFile.c_str(), serverName, portNum);

    int fds= connectClient (serverName, portNum);

    serverPacket pack;
    pack.id = idNum;
    strcpy(pack.packet, "HELLO");
    write(fds, (char *)&pack, sizeof(serverPacket));
    std::cout<<"Transmitted (src= "<<idNum<<") (HELLO, idNumber= "<<idNum<<")"<<std::endl;
    char helloMsg[3] = "OK";
    read(fds, (char *)&helloMsg, sizeof(helloMsg));
    std::cout<<"Received (src= 0) "<<helloMsg<<std::endl<<std::endl;


    std::ifstream infile(inputFile);
    if (!infile.is_open()) {
        std::cerr << "Failed to open input file: " << inputFile <<std::endl;
        exit(1);
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
        else if(token != NULL && (token[0] == '{' || token[0] == '}')){
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
            else if(packet.compare("quit") == 0){   // handle quit by sending DONE packet to server
                serverPacket cmd;
                memset((char *) &cmd, 0, sizeof(cmd));
                cmd.id = idNum;
                strcpy(cmd.packet, "DONE");
                sendToServer(fds, cmd);
                std::cout<<"Transmitted (src= "<<idNum<<") DONE"<<std::endl;
                char message[32];
                read(fds, (char *) &message, sizeof(message));
                std::cout<<"Received (src= 0) "<<message<<std::endl<<std::endl;
                break;
            }
            else if(packet.compare("gtime") == 0){
                serverPacket cmd;
                memset((char *) &cmd, 0, sizeof(cmd));
                cmd.id = idNum;
                strcpy(cmd.packet, packet.c_str());
                sendToServer(fds, cmd);
                std::cout<<"Transmitted (src= "<<idNum<<") GTIME"<<std::endl;
                
                char message[32];
                read(fds, (char *) &message, sizeof(message));
                std::cout<<"Received (src= 0) "<<message<<std::endl<<std::endl;
            }
            else if(packet.compare("put") == 0){
                serverPacket cmd;
                std::string thirdArg = token;
                memset((char *) &cmd, 0, sizeof(cmd));
                cmd.id = idNum;
                strcpy(cmd.packet, packet.c_str());
                strcpy(cmd.objectName, thirdArg.c_str());
                std::string fileLines;
                std::getline(infile, fileLines);
                int count = 0;
                while(fileLines.compare("}") != 0){
                    if(fileLines.compare("{") == 0) { 
                        std::getline(infile, fileLines);
                        continue; 
                    }

                    // for storing the files contents
                    if(count == 0){
                        std::string newLine = "  [0]: '";
                        newLine = newLine + fileLines + "'";
                        strcpy(cmd.line1, newLine.c_str());
                        count++;
                    }
                    else if(count == 1){
                        std::string newLine = "  [1]: '";
                        newLine = newLine + fileLines + "'";
                        strcpy(cmd.line2, newLine.c_str());
                        count++;
                    }
                    else if(count == 2){
                        std::string newLine = "  [2]: '";
                        newLine = newLine + fileLines + "'";
                        strcpy(cmd.line3, newLine.c_str());
                        count++;
                    }
                    std::getline(infile, fileLines);
                }
                sendToServer(fds, cmd);
                std::cout<<"Transmitted (src= "<<idNum<<") (PUT: "<<thirdArg<<")"<<std::endl;
                if(count >= 1){
                    printf("%s\n", cmd.line1); 
                }
                if(count >= 2){
                    printf("%s\n", cmd.line2);
                }
                if(count >= 3){
                    printf("%s\n", cmd.line3);
                }

                char message[32];
                read(fds, (char *) &message, sizeof(message));
                std::cout<<"Received (src= 0) "<<message<<std::endl<<std::endl;
            }
            else{
                std::string thirdArg = token;
                serverPacket cmd;
                memset((char *) &cmd, 0, sizeof(cmd));
                cmd.id = idNum;
                strcpy(cmd.packet, packet.c_str());
                strcpy(cmd.objectName, thirdArg.c_str());
                sendToServer(fds, cmd);
                std::cout<<"Transmitted (src= "<<idNum<<") ("<<boost::to_upper_copy(packet)<<": "<<thirdArg<<")"<<std::endl;
                
                if(packet == "get"){
                    serverPacket ack;
                    read(fds, (char *)&ack, sizeof(serverPacket));
                    std::cout<<"Received (src= 0) "<<ack.packet<<std::endl;
                    if(strcmp(ack.packet, "OK") == 0){
                        
                        if(ack.line1[0] != '\0'){
                            std::cout << ack.line1 << std::endl;
                        }
                        if (ack.line2[0] != '\0')
                        {
                            std::cout << ack.line2 << std::endl;
                        }
                        if (ack.line3[0] != '\0')
                        {
                            std::cout << ack.line3 << std::endl;
                        }
                    }
                    std::cout<<std::endl;
                    
                    // use serverPacket type to read instead of reading amessage.
                }
                else{
                    char message[32];
                    read(fds, (char *) &message, sizeof(message));
                    std::cout<<"Received (src= 0) "<<message<<std::endl<<std::endl;
                }

            }
        }
    }

    // close socket when done
    std::cout<<"do_client: client closing connection"<<std::endl<<std::endl;
    close(fds);
    infile.close();

    end = times(&tms_end);
    printf("\nreal: %7.2f sec.\nuser: %7.2f sec.\nsys : %7.2f sec.\n",
        (end-start) / (double)tics_per_second,
        (tms_end.tms_utime - tms_start.tms_utime) / (double)tics_per_second,
        (tms_end.tms_stime - tms_start.tms_stime) / (double)tics_per_second);
}

void do_server(int portNum){
    std::cout<<"a3w23: do_server."<<std::endl;

    // start recording time
    long tics_per_second;
    tics_per_second = sysconf(_SC_CLK_TCK);
    struct tms tms_start, tms_end;
    clock_t start, end;
    start = times(&tms_start);

    std::vector<storedInfo> infoReceived;       // used to store client puts
    struct pollfd       fds[NCLIENT+2];
    struct sockaddr_in  from;
    int   newsock[NCLIENT+1];
    socklen_t  fromlen;
    int done[NCLIENT + 2];

    for (int i= 0; i <= NCLIENT; i++) done[i]= -1;
    
    fds[0].fd=  serverListen (portNum, NCLIENT);
    fds[0].events= POLLIN;
    fds[0].revents= 0;
    fds[4].fd= STDIN_FILENO;
    fds[4].events= POLLIN;
    fds[4].revents = 0;

    std::cout<<"Server is accepting connections (port= "<<portNum<<")"<<std::endl<<std::endl;

    bool quit = false;
    int ret;
    
    int N = 1;
    while(!quit){
        
        ret = poll(fds, 5, 0);
        if(ret > 0){    // event has occured in something
        
            if ( (N < NCLIENT+1) && (fds[0].revents & POLLIN) ) {
            // accept a new connection
                fromlen= sizeof(from);
                newsock[N]= accept(fds[0].fd, (SA *) &from, &fromlen);
                fds[N].fd= newsock[N];
                fds[N].events= POLLIN;
                fds[N].revents= 0;
                done[N] = 0;
                N++;
            }

            for(int i = 1; i <= N-1; i++){
                if((fds[i].revents && POLLIN) && done[i] == 0){        //read from fifo
                    
                    fds[i].revents = 0;
                    serverPacket inPacket;
                    memset((char *) &inPacket, 0, sizeof(inPacket) );

                    int check = read(fds[i].fd, (char *) &inPacket, sizeof(inPacket));
                    
                    if(check == 0 && done[i] == 0){     // if empty packet sent and socket is not done yet, a CTRL + C has been used.
                        std::cerr<<"Receiving packet has zero length."<<std::endl;
                        std::cout<<"Server lost connection with client "<<i<<std::endl<<std::endl;
                        done[i] = 1;    // set status to done
                        close(fds[i].fd);
                        continue;
                    }
                    else if(check != sizeof(inPacket)){     // otherwise check if error occured with read.
                        std::cerr<<"Receiving packet length different from expected."<<std::endl;
                    }
                    
                    
                    // Code for when packet is put
                    if(strcmp(inPacket.packet,"put") == 0){  // code for PUT packet
                        std::cout<<"Received (src= "<<inPacket.id<<") (PUT: "<<(inPacket.objectName)<<")"<<std::endl;
                        int count = 0;
                        if(inPacket.line1[0] != '\0'){
                            std::cout<<inPacket.line1<<std::endl;
                            count++;
                        }
                        if(inPacket.line2[0] != '\0'){
                            std::cout<<inPacket.line2<<std::endl;
                            count++;
                        }
                        if(inPacket.line3[0] != '\0'){
                            std::cout<<inPacket.line3<<std::endl;
                            count++;
                        }
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
                            if(count >= 1){
                                strcpy(temp.line1, inPacket.line1);
                            }
                            if(count >= 2){
                                strcpy(temp.line2, inPacket.line2);
                            }
                            if(count >= 3){
                                strcpy(temp.line3, inPacket.line3);
                            }
                            infoReceived.push_back(temp);
                            message = "OK";
                            strcpy(msg2, message.c_str());
                            write(fds[i].fd, &msg2, sizeof(msg2));
                        }
                        else{
                            message = "(ERROR: object already exists)";
                            strcpy(msg2, message.c_str());
                            write(fds[i].fd, &msg2, sizeof(msg2));
                        }
                        std::cout<<"Transmitted (src= 0) "<<msg2<<std::endl<<std::endl;
                    }
                    else if(strcmp(inPacket.packet, "HELLO") == 0){
                        std::cout<<"Received (src= "<<inPacket.id<<") (HELLO, idNumber= "<<inPacket.id<<")"<<std::endl;
                        char msg2[3] = "OK";
                        write(fds[i].fd, &msg2, sizeof(msg2));
                        std::cout<<"Transmitted (src= 0) "<<msg2<<std::endl<<std::endl;
                    }
                    else if(strcmp(inPacket.packet, "DONE") == 0){
                        std::cout<<"Received (src= "<<inPacket.id<<") "<<(inPacket.packet)<<std::endl;
                        char msg2[3] = "OK";
                        done[i] = 1;
                        write(fds[i].fd, &msg2, sizeof(msg2));
                        std::cout<<"Transmitted (src= 0) "<<msg2<<std::endl<<std::endl;
                    }
                    else if(strcmp(inPacket.packet,"get") == 0){ // code for GET packet
                        std::cout<<"Received (src= "<<inPacket.id<<") (GET: "<<(inPacket.objectName)<<")"<<std::endl;
                        bool exists = false;
                        int j;
                        for(j = 0; j < (int)infoReceived.size(); j++){
                            if(strcmp(infoReceived[j].objectName,inPacket.objectName) == 0){  // check if object name already exists in server
                                exists = true;
                                break;
                            }
                        }

                        serverPacket ack;
                        memset((char *) &ack, 0, sizeof(ack));
                        if(exists){     // get if it does exist
                            ack.id = 0;
                            strcpy(ack.packet, "OK");
                            strcpy(ack.line1, infoReceived[j].line1);
                            strcpy(ack.line2, infoReceived[j].line2);
                            strcpy(ack.line3, infoReceived[j].line3);
                            write(fds[i].fd, (char *) &ack, sizeof(serverPacket));
                        }
                        else{   // send error if file does not exist
                            strcpy(ack.packet, "(ERROR: object not found)");
                            write(fds[i].fd, (char *) &ack, sizeof(serverPacket));
                        }
                        std::cout<<"Transmitted (src= 0) "<<ack.packet<<std::endl<<std::endl;
                    }
                    else if(strcmp(inPacket.packet,"delete") == 0){  // code for delete packet
                        std::cout<<"Received (src= "<<inPacket.id<<") (DELETE: "<<(inPacket.objectName)<<")"<<std::endl;
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
                                write(fds[i].fd, &msg2, sizeof(msg2));
                            }
                            else{
                                message = "(ERROR: client not owner)";
                                strcpy(msg2, message.c_str());
                                write(fds[i].fd, &msg2, sizeof(msg2));
                            }
                            
                        }
                        else{   // send error if file does not exist
                            message = "(ERROR: object not found)";
                            strcpy(msg2, message.c_str());
                            write(fds[i].fd, &msg2, sizeof(msg2));
                        }
                        std::cout<<"Transmitted (src= server) "<<msg2<<std::endl<<std::endl;
                    }
                    else if(strcmp(inPacket.packet,"gtime") == 0){   // code for GTIME packet
                        std::cout<<"Received (src= "<<inPacket.id<<") GTIME"<<std::endl;
                        char msg2[32];
                        struct tms tms_curr;
                        clock_t currTime = times(&tms_curr);
                        double timeNum = (currTime - start)/(double)tics_per_second;
                        
                        std::stringstream ss;
                        ss << std::fixed << std::setprecision(2) << timeNum;
                        std::string time = ss.str();
                        std::string message = "(TIME:   " + time + ")";
                        strcpy(msg2, message.c_str());

                        write(fds[i].fd, &msg2, sizeof(msg2));
                        std::cout<<"Transmitted (src= 0) "<<msg2<<std::endl<<std::endl;
                    }
                    else{
                        std::cout<<"Received (src= "<<inPacket.id<<") "<<inPacket.packet<<std::endl;
                        std::string message = "(ERROR: invalid packet type)";
                        
                        char msg2[32];
                        strcpy(msg2, message.c_str());
                        write(fds[i].fd, &msg2, sizeof(msg2));
                        std::cout<<"Transmitted (src= 0) "<<msg2<<std::endl<<std::endl;
                    }
                }
            }
            if(fds[4].revents & POLLIN){
                fds[4].revents = 0;
                char userInput[5];
                read(fds[4].fd, (char *) &userInput, sizeof(userInput));

                userInput[strcspn(userInput, "\n")] = 0;

                if(strcmp(userInput,"list") == 0){
                    std::cout<<"Object table:"<<std::endl;
                    for(auto client : infoReceived){
                        std::cout<<"(owner= "<<client.id<<", name= "<<client.objectName<<")"<<std::endl;
                        

                        if(client.line1[0] != '\0'){
                        std::cout << client.line1 << std::endl;

                        }
                        if (client.line2[0] != '\0')
                        {
                            std::cout << client.line2 << std::endl;

                        }
                        if (client.line3[0] != '\0')
                        {
                            std::cout << client.line3 << std::endl;

                        }
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

    std::cout<<"do_server: server closing main socket (";
    for(int i = 1; i < NCLIENT + 1; i++){
        if(done[i] == 1){
           printf("done[%d]= %d, ", i, done[i]); 
        }
    }
    std::cout<<")"<<std::endl;

    end = times(&tms_end);
    printf("\nreal: %7.2f sec.\nuser: %7.2f sec.\nsys : %7.2f sec.\n",
        (end-start) / (double)tics_per_second,
        (tms_end.tms_utime - tms_start.tms_utime) / (double)tics_per_second,
        (tms_end.tms_stime - tms_start.tms_stime) / (double)tics_per_second);

    for (int i=1; i <= N-1; i++) close(fds[i].fd);
    close(fds[0].fd);
    close(fds[1].fd);
    
}

int main(int argc, char *argv[]){
    if(argc == 1){      // error message if invalid arguments passed
        std::cout<<"Usage: "<<argv[0]<<" -s"<<"   Or     Usage: "<<argv[0]<<" -c idNumber inputFile";
        exit(1);
    }
    int portNum;

    if(argc == 3 && (strcmp(argv[1], "-s") == 0)){    // run server if -s entered
        portNum = atoi(argv[2]);
        do_server(portNum);
    }
    else if(argc == 6 && (strcmp(argv[1], "-c") == 0)){      // run client if -c entered
        int idNum = atoi(argv[2]);
        std::string inputFile = argv[3];
        portNum = atoi(argv[5]);
        do_client(inputFile, idNum, argv[4], portNum);
    }
    else{
        std::cout<<" Usage: "<<argv[0]<<" -s portnum    OR   Usage: "<<argv[0]<<" -c idNum inputFile serverName portNum"<<std::endl;
        exit(1);
    }
    

    return 0;
    
}