#include "Arduino_Node.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Uso: ./Arduino_Node <IP_NODO_MAESTRO> <PUERTO> [SERIAL_PATH|stdin|simulate] [format=json|binary|both]\n";
        return 1;
    }

    std::string masterIP = argv[1];
    int masterPort = std::stoi(argv[2]);
    std::string serialPath = "";
    if (argc >= 4) serialPath = argv[3];
    std::string format = "json";
    if (argc >= 5) format = argv[4];

    ArduinoNode node(masterIP, masterPort, serialPath, format);
    node.run();
    return 0;
}
