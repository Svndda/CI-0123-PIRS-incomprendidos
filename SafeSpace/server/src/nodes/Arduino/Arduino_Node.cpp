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
#include <map>
#include <sstream>
#include <iomanip>

// Base serial port if none specified
static const char* DEFAULT_SERIAL_PORT = "/dev/ttyACM0";
static const speed_t BAUD = B9600;

// Datagrama SENSOR_DATA (extended)
#pragma pack(push, 1)
struct SensorPacket {
    uint8_t  msgId;          // 0x42 = SENSOR_DATA
    int16_t  temp_x100;      // temperatura * 100 (network byte order when sent)
    int16_t  hum_x100;       // humedad * 100 (network byte order when sent)
    int16_t  distance_x100;  // distance units *100 (e.g., meters*100) as signed int16
    int32_t  pressure_pa;    // pressure in Pascals as 32-bit integer (network byte order)
    int16_t  altitude_x100;  // altitude *100 (meters) as signed int16
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

ArduinoNode::ArduinoNode(const std::string& masterIP, int masterPort, const std::string& serialPath, const std::string& format)
    : masterIP_(masterIP), masterPort_(masterPort), serialPath_(serialPath), running_(false) {
    std::string f = format;
    for (auto &c : f) c = std::tolower((unsigned char)c);
    if (f == "binary") format_ = FORMAT_BINARY; else format_ = FORMAT_JSON;
}

ArduinoNode::~ArduinoNode() {
}

void ArduinoNode::stop() {
    running_.store(false);
}

// send raw JSON text via UDP
void ArduinoNode::sendJson(const std::string& json, int usock) {
    ssize_t sent = sendto(usock, json.data(), json.size(), 0,
                          reinterpret_cast<sockaddr*>(&dst_), sizeof(dst_));
    if (sent < 0)
        perror("sendto");
    else
        std::cout << "[Arduino_Node] JSON sent: " << json << "\n";
}

void ArduinoNode::sendBinarySensor(double temp, double hum, double distance, double pressure, double altitude, int usock) {
    SensorPacket p{};
    p.msgId = 0x42;
    // convert to int16 (temp * 100, hum * 100)
    int16_t t = static_cast<int16_t>(std::lround(temp * 100.0));
    int16_t h = static_cast<int16_t>(std::lround(hum * 100.0));
    int16_t d = static_cast<int16_t>(std::lround(distance * 100.0));
    int32_t pr = static_cast<int32_t>(std::lround(pressure));
    int16_t al = static_cast<int16_t>(std::lround(altitude * 100.0));

    p.temp_x100 = htons(static_cast<uint16_t>(t));
    p.hum_x100 = htons(static_cast<uint16_t>(h));
    p.distance_x100 = htons(static_cast<uint16_t>(d));
    p.pressure_pa = htonl(static_cast<uint32_t>(pr));
    p.altitude_x100 = htons(static_cast<uint16_t>(al));

    ssize_t sent = sendto(usock, reinterpret_cast<const char*>(&p), sizeof(p), 0,
                          reinterpret_cast<sockaddr*>(&dst_), sizeof(dst_));
    if (sent < 0) perror("sendto");
    else std::cout << "[Arduino_Node] Binary packet sent (msgId=0x42)\n";
}

// parse a single label/value printed by the Arduino into normalized key and numeric string
static std::pair<std::string,std::string> parseLabelValue(const std::string& line) {
    auto trim = [](std::string s) {
        // trim both ends
        const char* ws = " \t\r\n";
        size_t start = s.find_first_not_of(ws);
        if (start == std::string::npos) return std::string();
        size_t end = s.find_last_not_of(ws);
        return s.substr(start, end - start + 1);
    };

    std::string key, val;
    size_t pos = line.find(":");
    if (pos != std::string::npos) {
        key = trim(line.substr(0, pos));
        val = trim(line.substr(pos+1));
    } else if ((pos = line.find("=")) != std::string::npos) {
        key = trim(line.substr(0, pos));
        val = trim(line.substr(pos+1));
    } else {
        // unstructured, put entire line as message
        key = "message";
        val = trim(line);
        return {key,val};
    }

    // remove units from val (keep numeric prefix)
    // find first char that's not part of number, + - . digits
    size_t i = 0;
    // allow optional leading sign
    if (i < val.size() && (val[i] == '+' || val[i] == '-')) ++i;
    bool seenDigit = false;
    while (i < val.size()) {
        char c = val[i];
        if ((c >= '0' && c <= '9') || c == '.') { seenDigit = true; ++i; continue; }
        // stop at first non-number
        break;
    }
    if (seenDigit) val = trim(val.substr(0, i));

    // normalize key names
    std::string kl = key;
    for (auto &c : kl) c = std::tolower((unsigned char)c);
    std::string nk = "";
    if (kl.find("distance") != std::string::npos) nk = "distance";
    else if (kl.find("humedad") != std::string::npos || kl.find("humidity") != std::string::npos) nk = "humidity";
    else if (kl.find("temperature") != std::string::npos) nk = "temperature";
    else if (kl.find("pressure at sealevel") != std::string::npos || kl.find("sealevel") != std::string::npos) nk = "pressure_sealevel";
    else if (kl.find("pressure") != std::string::npos) nk = "pressure";
    else if (kl.find("real altitude") != std::string::npos) nk = "real_altitude";
    else if (kl.find("altitude") != std::string::npos) nk = "altitude";
    else nk = key; // fallback keep original

    return {nk, val};
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

    std::map<std::string,std::string> block;
    while (running_.load()) {
        if (simulate) {
            double temp = tempDist(rng);
            double hum = humDist(rng);
            double dist = 100.0 * (rng() % 200) / 100.0; // simple random distance
            double press = 100000 + (rng() % 5000);
            double alt = 50.0 + (rng() % 200) / 10.0;

            if (format_ == FORMAT_JSON) {
                std::ostringstream js;
                js << std::fixed << std::setprecision(3);
                js << "{";
                js << "\"temperature\":" << temp << ",";
                js << "\"humidity\":" << hum << ",";
                js << "\"distance\":" << dist << ",";
                js << "\"pressure\":" << press << ",";
                js << "\"altitude\":" << alt;
                js << "}";
                sendJson(js.str(), usock);
            } else {
                // legacy binary: only temperature and humidity are sent
                sendBinarySensor(temp, hum, dist, press, alt, usock);
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        std::string line = readLine(sfd);
        // A blank line indicates end of a block from Arduino prints
        if (line.empty()) {
            if (!block.empty()) {
                if (format_ == FORMAT_JSON) {
                    // build JSON from collected key/values
                    std::ostringstream js;
                    js << "{";
                    bool first = true;
                    for (const auto& kv : block) {
                        if (!first) js << ",";
                        first = false;
                        // attempt numeric output when possible
                        const std::string& k = kv.first;
                        const std::string& v = kv.second;
                        // if v looks numeric, output without quotes
                        bool isnum = !v.empty() && (std::isdigit(v[0]) || v[0]=='+' || v[0]=='-' );
                        js << "\"" << k << "\":";
                        if (isnum) js << v; else js << "\"" << v << "\"";
                    }
                    js << "}";

                    sendJson(js.str(), usock);
                } else {
                    // binary mode: only extract temperature and humidity from block
                    double temp = 0.0, hum = 0.0;
                    bool haveTemp = false, haveHum = false;
                    auto it = block.find("temperature");
                    if (it != block.end()) { try { temp = std::stod(it->second); haveTemp = true; } catch(...){} }
                    it = block.find("humidity");
                    if (it != block.end()) { try { hum = std::stod(it->second); haveHum = true; } catch(...){} }
                    double dist = 0.0, press = 0.0, alt = 0.0;
                    auto it2 = block.find("distance");
                    if (it2 != block.end()) { try { dist = std::stod(it2->second); } catch(...){} }
                    it2 = block.find("pressure");
                    if (it2 != block.end()) { try { press = std::stod(it2->second); } catch(...){} }
                    it2 = block.find("altitude");
                    if (it2 != block.end()) { try { alt = std::stod(it2->second); } catch(...){} }
                    if (haveTemp && haveHum) sendBinarySensor(temp, hum, dist, press, alt, usock);
                    else std::cerr << "[Arduino_Node] binary format requested but temperature/humidity missing in block\n";
                }
                block.clear();
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            continue;
        }

        // parse a label line and store into block
        auto kv = parseLabelValue(line);
        if (!kv.first.empty()) block[kv.first] = kv.second;
        // continue reading until blank line
    }

    close(usock);
    if (!simulate && sfd >= 0) close(sfd);
}

// Note: no main() here so this file remains a class implementation only.
// For standalone testing, see Arduino_Node_main.cpp which provides a small
// executable wrapper that instantiates ArduinoNode and calls run().