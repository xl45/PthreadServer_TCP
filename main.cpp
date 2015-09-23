#include <iostream>
#include "PthreadServer.h"

int main(){
    PthreadServer myServer("6002");
    myServer.acceptConnections();

    return 0;
}

