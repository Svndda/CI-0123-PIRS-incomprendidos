// #include <iostream>
// #include <cstring>
// #include <cstdlib>
// #include <unistd.h>
// #include <arpa/inet.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <errno.h>
// #include <thread>
// #include <chrono>
//
// // Incluir la definición de SensorData
// #include "../../model/structures/sensordata.h"
//
// // Configuración
// static const int BUFFER_SIZE = 1024;
//
// // Estructura del paquete recibido de Arduino (debe coincidir con Arduino_Node)
// #pragma pack(push, 1)
// struct SensorPacket {
//     uint8_t  msgId;     // 0x42 = SENSOR_DATA
//     int16_t  temp_x100; // temperatura * 100 (network byte order)
//     int16_t  hum_x100;  // humedad * 100 (network byte order)
// };
// #pragma pack(pop)
//
// // Funciones de utilidad
// int createUdpSocket(int port) {
//     int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sockfd < 0) {
//         perror("socket");
//         std::exit(1);
//     }
//
//     sockaddr_in server_addr{};
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(port);
//     server_addr.sin_addr.s_addr = INADDR_ANY;
//
//     if (bind(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
//         perror("bind");
//         close(sockfd);
//         std::exit(1);
//     }
//
//     std::cout << "[Intermediary_Node] Escuchando en puerto " << port << std::endl;
//     return sockfd;
// }
//
// void setupMasterConnection(const std::string& master_ip, int master_port) {
//     std::cout << "[Intermediary_Node] Configurado para enviar a Master_node en "
//               << master_ip << ":" << master_port << std::endl;
//
//     // Por ahora solo mostramos la configuración
//     // Cuando el Master_node esté listo, crearemos la conexión aquí
// }
//
// void processSensorPacket(const SensorPacket& packet) {
//     // Convertir de network byte order a host byte order
//     int16_t temp_raw = ntohs(static_cast<uint16_t>(packet.temp_x100));
//     int16_t hum_raw = ntohs(static_cast<uint16_t>(packet.hum_x100));
//
//     // Convertir a valores reales
//     double temperature = static_cast<double>(temp_raw) / 100.0;
//     double humidity = static_cast<double>(hum_raw) / 100.0;
//
//     std::cout << "[Intermediary_Node] Datos recibidos - Temp: " << temperature
//               << "°C, Hum: " << humidity << "%" << std::endl;
//
//     // Crear objeto SensorData (convertir double a int para la estructura)
//     // Nota: Solo tenemos temperatura y humedad del Arduino, otros valores se dejan en -1
//     SensorData sensorData(
//         -1,  // distance
//         -1,  // movement
//         static_cast<int>(temperature),  // temperature
//         -1,  // uv
//         -1,  // microphone
//         -1,  // led
//         -1,  // buzzer
//         -1   // light
//     );
//
//     // Enviar datos al Master_node
//     // Por ahora simplemente imprimimos que se enviaría
//     std::cout << "[Intermediary_Node] Preparado para enviar SensorData al Master_node:" << std::endl;
//     std::cout << "[Intermediary_Node] - Temperatura: " << sensorData.temperature << std::endl;
//     //Descomentar esta linea para probar con el master
//     // send(master_sock, &sensorData, sizeof(sensorData), 0);
// }
//
// int main(int argc, char* argv[]) {
//     if (argc != 4) {
//         std::cerr << "Uso: " << argv[0] << " <PUERTO_ESCUCHA> <MASTER_IP> <MASTER_PUERTO>" << std::endl;
//         std::cerr << "Ejemplo: " << argv[0] << " 9999 192.168.1.100 8888" << std::endl;
//         return 1;
//     }
//
//     int listen_port = std::stoi(argv[1]);
//     std::string master_ip = argv[2];
//     int master_port = std::stoi(argv[3]);
//
//     std::cout << "[Intermediary_Node] Iniciando..." << std::endl;
//     std::cout << "[Intermediary_Node] Puerto escucha: " << listen_port << std::endl;
//     std::cout << "[Intermediary_Node] Master node: " << master_ip << ":" << master_port << std::endl;
//
//     // Crear socket para escuchar Arduino_node
//     int listen_sock = createUdpSocket(listen_port);
//
//     // Configurar conexión con Master_node (por ahora solo configuración)
//     setupMasterConnection(master_ip, master_port);
//
//     std::cout << "[Intermediary_Node] Listo para recibir datos de Arduino_node..." << std::endl;
//
//     // Bucle principal
//     while (true) {
//         char buffer[BUFFER_SIZE];
//         sockaddr_in client_addr{};
//         socklen_t client_len = sizeof(client_addr);
//
//         // Recibir datos de Arduino_node
//         ssize_t n = recvfrom(listen_sock, buffer, sizeof(buffer), 0,
//                            reinterpret_cast<sockaddr*>(&client_addr), &client_len);
//
//         if (n < 0) {
//             perror("recvfrom");
//             continue;
//         }
//
//         // Verificar que tenemos un paquete completo
//         if (n == sizeof(SensorPacket)) {
//             SensorPacket* packet = reinterpret_cast<SensorPacket*>(buffer);
//
//             if (packet->msgId == 0x42) {  // SENSOR_DATA
//                 processSensorPacket(*packet);
//             } else {
//                 std::cerr << "[Intermediary_Node] ID de mensaje desconocido: 0x"
//                           << std::hex << static_cast<int>(packet->msgId) << std::dec << std::endl;
//             }
//         } else {
//             std::cerr << "[Intermediary_Node] Paquete de tamaño incorrecto: " << n
//                       << " bytes (esperaba " << sizeof(SensorPacket) << ")" << std::endl;
//         }
//     }
//
//     close(listen_sock);
//     return 0;
// }