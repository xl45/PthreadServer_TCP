#include "PthreadServer.h"

fd_set readset, tempset;
// for the pthread
int fd_holder[10];

// handler function for each readable connection, will be passed to the created thread
void * service( void * arg );
// parse HTTP GET
std::string get_req_filename(char * recv_buffer);
// parse HTTP GET
bool check_conn_field(char * recv_buffer);



PthreadServer::PthreadServer(std::string port)
{
    // set the struct
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status;
    // getaddrinfo
    if( (status = getaddrinfo(NULL, port.c_str(), &hints, &serverinfo)) != 0 ){
        std::cout << "getaddrinfo() function error:" << gai_strerror(status) << std::endl;
        exit(1);
    }
    // get the fd
    if( (serverFD = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol)) == -1){
        perror("server socket(): ");
        exit(1);
    }
    // bind to port
    if( bind(serverFD, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1 ){
        perror("server bind(): ");
        exit(1);
    }
    // listen to port
    if( listen(serverFD, 10) == -1 ){
        perror("server listen(): ");
        exit(1);
    }
    // for select
    FD_ZERO(&readset);
    FD_ZERO(&tempset);
    FD_SET(serverFD, &readset);
    maxFD = serverFD;
    tv.tv_sec = 60;
    tv.tv_usec = 0;
}


void PthreadServer::acceptConnections(){
    do{
        memcpy(&tempset, &readset, sizeof(tempset));

        int nReady = select(maxFD+1, &tempset, NULL, NULL, &tv);

        if( nReady == 0 ){
            std::cout << "select() timeout, continue now.\n";
            continue;
        }else if( nReady < 0 ){
            perror("server select(): ");
            exit(1);
        }else{
            // if new incoming connection
            if( FD_ISSET(serverFD, &tempset) ){
                int addr_len = sizeof(client_addr);
                int clientFD = accept(serverFD, (sockaddr*)&client_addr, (socklen_t*)&addr_len);

                // add client fd into readset
                if( clientFD < 0 ){
                    perror("server accept(): ");
                    continue;
                }else{
                    FD_SET(clientFD, &readset);
                    maxFD = (maxFD < clientFD) ? clientFD:maxFD;
                    std::cout << "incoming fd: " << clientFD << " accepted.\n";
                }

                continue;
            }

            // traversal the tempset
            int fd;
            for( fd = 0; fd < maxFD+1; fd++ ){
                if( FD_ISSET(fd, &tempset) ){
                    // remove fd from readset
                    FD_CLR(fd, &readset);

                    // set the attributes to detach mode
                    pthread_attr_t attr;
                    int ret;
                    ret = pthread_attr_init(&attr);
                    ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

                    // create detached pthread and pass handler function pointer to it
                    fd_holder[fd - 1] = fd;
                    pthread_create(&thread[fd], &attr, service, (void*)&(fd_holder[fd-1]) );
                }
            }
        }
    } while( 1 );
}


void * service( void * arg ){
    int tempclientFD = *((int*)arg);
    char recv_buffer[RECV_BUF_MAX_SIZE];

    // recv and send
    while(1){
        memset(recv_buffer, 0, RECV_BUF_MAX_SIZE);

        if( recv(tempclientFD, &recv_buffer, RECV_BUF_MAX_SIZE, 0) == -1 ){
            perror("server recv(): ");
            continue;
        }

        // get the request file name
        std::string req_filename = get_req_filename(recv_buffer);
        if( req_filename == "badreq" ){
            // std::cout << "bad request, continue to recv()\n";
            continue;
        }
        std::cout << "client socket " << tempclientFD << " HTTP request: GET " << req_filename << std::endl;

        // if not found send 404, else send the file data
        FILE * pFile;
        if( (pFile = fopen(req_filename.c_str() ,"r")) == NULL ){
            // 404 not found
            std::string page = "404 not found!";
            if( (send(tempclientFD, page.c_str(), page.length(), 0)) == -1 ){
                perror("server send(): ");
                exit(1);
            }
        }else{
            // obtain file size:
            fseek (pFile , 0 , SEEK_END);
            long file_size = ftell(pFile);
            rewind (pFile);
            // read the file into buffer
            BYTE buffer[file_size];
            fread(buffer, 1, file_size, pFile);
            // send the file
            if( (send(tempclientFD, buffer, file_size, 0)) == -1 ){
                perror("server send(): ");
                exit(1);
            }
            fclose(pFile);
        }

        // check if close or persistent
        if( check_conn_field(recv_buffer) ){
            continue;
        }else{
            // close connection and end thread
            close(tempclientFD);
            pthread_exit(0);
        }
    }
}


std::string get_req_filename(char * recv_buffer){
    std::string whole_msg( recv_buffer );

    std::string substr1("GET /");
    std::string substr2(" HTTP/1.1");

    std::size_t found1 = whole_msg.find(substr1);
    std::size_t found2 = whole_msg.find(substr2);

    if( (int)found1 == -1 || (int)found2 == -1 ){
        // bad request
        return "badreq";
    }

    std::string req_filename = whole_msg.substr(found1+substr1.length(), found2-found1-substr1.length());

    return req_filename;
}


bool check_conn_field(char * recv_buffer){
    std::string whole_msg(recv_buffer);
    std::string substr1("keep-alive");

    std::size_t found1 = whole_msg.find(substr1);

    if( (int)found1 == -1 ){
        return false;
    }else{
        return true;
    }
}


PthreadServer::~PthreadServer()
{
    //dtor
}
