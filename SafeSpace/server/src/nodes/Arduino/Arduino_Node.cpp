#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>

// CONFIGURACIÓN BASE
static const char* SERIAL_PORT = "/dev/ttyACM0";
static const speed_t BAUD = B9600;

// Datagrama SENSOR_DATA
#pragma pack(push, 1)
struct SensorPacket {
    uint8_t  msgId;     // 0x42 = SENSOR_DATA
    int16_t  temp_x100; // temperatura * 100
    int16_t  hum_x100;  // humedad * 100
};
#pragma pack(pop)

// Funciones
bool parseJsonLine(const std::string& line, double& temp, double& hum) {
    auto tpos = line.find("\"temp\":");
    auto hpos = line.find("\"humidity\":");
    if (tpos == std::string::npos || hpos == std::string::npos)
        return false;
    try {
        temp = std::stod(line.substr(tpos + 7));
        hum  = std::stod(line.substr(hpos + 11));
        return true;
    } catch (...) { return false; }
}

int openSerial() {
    int fd = open(SERIAL_PORT, O_RDONLY | O_NOCTTY);
    if (fd < 0) { perror("open serial"); std::exit(1); }

    termios tty{};
    tcgetattr(fd, &tty);
    cfsetispeed(&tty, BAUD);
    cfsetospeed(&tty, BAUD);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE; tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_iflag = 0;
    tcsetattr(fd, TCSANOW, &tty);

    std::cout << "Serial abierto: " << SERIAL_PORT << "\n";
    return fd;
}

int openUdpSock(sockaddr_in& dst, const std::string& ip, int port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); std::exit(1); }

    dst = {};
    dst.sin_family = AF_INET;
    dst.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &dst.sin_addr);
    return sock;
}

std::string readLine(int fd) {
    std::string line;
    char c;
    while (true) {
        ssize_t n = read(fd, &c, 1);
        if (n <= 0) break;
        if (c == '\n') break;
        line.push_back(c);
    }
    return line;
}

// MAIN
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Uso: ./Arduino_Node <IP_NODO_MAESTRO> <PUERTO>\n";
        return 1;
    }

    std::string masterIP = argv[1];
    int masterPort = std::stoi(argv[2]);

    std::cout << "[Arduino_Node] Enviando lecturas a " << masterIP << ":" << masterPort << "\n";

    int sfd = openSerial();
    sockaddr_in dst{};
    int usock = openUdpSock(dst, masterIP, masterPort);

    while (true) {
        std::string line = readLine(sfd);
        if (line.empty()) { 
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        double temp=0, hum=0;
        if (!parseJsonLine(line, temp, hum)) {
            std::cerr << "[Arduino_Node] Formato inválido: " << line << "\n";
            continue;
        }

        SensorPacket pkt{0x42, static_cast<int16_t>(temp*100), static_cast<int16_t>(hum*100)};
        ssize_t sent = sendto(usock, &pkt, sizeof(pkt), 0,
                              reinterpret_cast<sockaddr*>(&dst), sizeof(dst));

        if (sent < 0)
            perror("sendto");
        else
            std::cout << "[Arduino_Node] SENSOR_DATA temp=" << temp << " hum=" << hum << " enviado.\n";
    }

    close(usock);
    close(sfd);
    return 0;
}