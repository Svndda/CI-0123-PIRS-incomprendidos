#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <thread>
#include <chrono>

// CONFIGURACIÓN BASE
static const char* SERIAL_PORT = "/dev/ttyACM0";
static const speed_t BAUD = B9600;

// Datagrama SENSOR_DATA
#pragma pack(push, 1)
struct SensorPacket {
    uint8_t  msgId;     // 0x42 = SENSOR_DATA
    int16_t  temp_x100; // temperatura * 100 (network byte order when sent)
    int16_t  hum_x100;  // humedad * 100 (network byte order when sent)
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
    if (tcgetattr(fd, &tty) != 0) { perror("tcgetattr"); close(fd); std::exit(1); }
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
    if (tcsetattr(fd, TCSANOW, &tty) != 0) { perror("tcsetattr"); close(fd); std::exit(1); }

    std::cout << "Serial abierto: " << SERIAL_PORT << "\n";
    return fd;
}

int openUdpSock(sockaddr_in& dst, const std::string& ip, int port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); std::exit(1); }

    dst = {};
    dst.sin_family = AF_INET;
    dst.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &dst.sin_addr) != 1) {
        std::cerr << "inet_pton failed for IP: " << ip << "\n";
        close(sock);
        std::exit(1);
    }
    return sock;
}

std::string readLine(int fd) {
    std::string line;
    char c;
    while (true) {
        ssize_t n = read(fd, &c, 1);
        if (n <= 0) break;
        if (c == '\n') break;
        if (c == '\r') continue; // ignore CR (handle CRLF from some Arduinos)
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

    // Convert sensor values to integers and to network byte order
    int16_t tval = static_cast<int16_t>(temp * 100);
    int16_t hval = static_cast<int16_t>(hum * 100);
    SensorPacket pkt{0x42, static_cast<int16_t>(htons(static_cast<uint16_t>(tval))),
             static_cast<int16_t>(htons(static_cast<uint16_t>(hval)))};
    ssize_t sent = sendto(usock, reinterpret_cast<const void*>(&pkt), sizeof(pkt), 0,
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