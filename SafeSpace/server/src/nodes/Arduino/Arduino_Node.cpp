#include "Arduino_Node.h"

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
#include <random>

// Base serial port if none specified
static const char* DEFAULT_SERIAL_PORT = "/dev/ttyACM0";
static const speed_t BAUD = B9600;

// Datagrama SENSOR_DATA
#pragma pack(push, 1)
struct SensorPacket {
    uint8_t  msgId;           // 0x42 = SENSOR_DATA
    float    distance;        // distancia en cm
    float    temperature;     // temperatura en °C
    float    pressure;        // presión en Pa
    float    altitude;        // altitud en metros
    float    sealevelPressure; // presión a nivel del mar en Pa
    float    realAltitude;    // altitud real en metros
};
#pragma pack(pop)

bool ArduinoNode::parseJsonLine(const std::string& line, double& temp, double& hum) {
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

int ArduinoNode::openSerial(const std::string& path) {
    // If user requested stdin, just return STDIN_FILENO (no termios changes)
    if (!path.empty() && path == "stdin") {
        std::cout << "Usando stdin como fuente de datos\n";
        return STDIN_FILENO;
    }

    const char* dev = path.empty() ? DEFAULT_SERIAL_PORT : path.c_str();
    int fd = open(dev, O_RDONLY | O_NOCTTY);
    if (fd < 0) { perror("open serial"); return -1; }

    termios tty{};
    if (tcgetattr(fd, &tty) != 0) { perror("tcgetattr"); close(fd); return -1; }
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
    if (tcsetattr(fd, TCSANOW, &tty) != 0) { perror("tcsetattr"); close(fd); return -1; }

    std::cout << "Serial abierto: " << dev << "\n";
    return fd;
}

int ArduinoNode::openUdpSock() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return -1; }

    dst_ = {};
    dst_.sin_family = AF_INET;
    dst_.sin_port = htons(masterPort_);
    if (inet_pton(AF_INET, masterIP_.c_str(), &dst_.sin_addr) != 1) {
        std::cerr << "inet_pton failed for IP: " << masterIP_ << "\n";
        close(sock);
        return -1;
    }
    return sock;
}

std::string ArduinoNode::readLine(int fd) {
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

ArduinoNode::ArduinoNode(const std::string& masterIP, int masterPort, const std::string& serialPath)
    : masterIP_(masterIP), masterPort_(masterPort), serialPath_(serialPath), running_(false) {}

ArduinoNode::~ArduinoNode() {
}

void ArduinoNode::stop() {
    running_.store(false);
}

void ArduinoNode::sendSensor(double temp, double hum, int usock) {
    int16_t tval = static_cast<int16_t>(temp * 100);
    int16_t hval = static_cast<int16_t>(hum * 100);
    SensorPacket pkt{0x42, static_cast<int16_t>(htons(static_cast<uint16_t>(tval))),
                     static_cast<int16_t>(htons(static_cast<uint16_t>(hval)))};
    ssize_t sent = sendto(usock, reinterpret_cast<const void*>(&pkt), sizeof(pkt), 0,
                          reinterpret_cast<sockaddr*>(&dst_), sizeof(dst_));

    if (sent < 0)
        perror("sendto");
    else
        std::cout << "[Arduino_Node] SENSOR_DATA temp=" << temp << " hum=" << hum << " enviado." << "\n";
}

void ArduinoNode::run() {
    std::cout << "[Arduino_Node] Enviando lecturas a " << masterIP_ << ":" << masterPort_ << "\n";

    bool simulate = (serialPath_ == "simulate");

    int sfd = -1;
    if (!simulate) {
        sfd = openSerial(serialPath_);
        if (sfd < 0) {
            std::cerr << "Failed to open serial source: " << serialPath_ << "\n";
            return;
        }
    }

    int usock = openUdpSock();
    if (usock < 0) {
        if (sfd >= 0) close(sfd);
        return;
    }

    running_.store(true);

    std::mt19937 rng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<double> tempDist(20.0, 28.0);
    std::uniform_real_distribution<double> humDist(40.0, 70.0);

    while (running_.load()) {
        double temp = 0.0, hum = 0.0;

        if (simulate) {
            temp = tempDist(rng);
            hum = humDist(rng);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } else {
            std::string line = readLine(sfd);
            if (line.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            if (!parseJsonLine(line, temp, hum)) {
                std::cerr << "[Arduino_Node] Formato inválido: " << line << "\n";
                continue;
            }
        }

        sendSensor(temp, hum, usock);
    }

    close(usock);
    if (!simulate && sfd >= 0) close(sfd);
}

// int main(int argc, char* argv[]) {
//     if (argc < 3) {
//         std::cerr << "Uso: ./Arduino_Node <IP_NODO_MAESTRO> <PUERTO> [SERIAL_PATH|stdin|simulate]\n";
//         return 1;
//     }
//
//     std::string masterIP = argv[1];
//     int masterPort = std::stoi(argv[2]);
//     std::string serialPath = "";
//     if (argc >= 4) serialPath = argv[3];
//
//     ArduinoNode node(masterIP, masterPort, serialPath);
//     node.run();
//     return 0;
// }