#include <iostream>
#include "PthreadServer.h"

int main(){
    PthreadServer myServer("3480");
    myServer.acceptConnections();

    return 0;
}

