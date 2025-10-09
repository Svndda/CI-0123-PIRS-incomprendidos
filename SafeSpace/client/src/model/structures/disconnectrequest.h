#pragma once
#include <array>
#include <cstdint>
#include <ostream>

/**
 * @class DisconnectRequest
 * @brief Representa la solicitud DISCONNECT enviada por un cliente o sensor
 *        para finalizar una sesión previamente establecida.
 */
class DisconnectRequest
{
public:
    DisconnectRequest() = default;

    DisconnectRequest(std::uint16_t sessionId,
                      std::uint16_t sensorId,
                      std::uint8_t flagBits) noexcept;

    Getters
        std::uint16_t
        getSessionId() const noexcept;
    std::uint16_t getSensorId() const noexcept;
    std::uint8_t getFlagBits() const noexcept;

    /* // Setters
void setSessionId(std::uint16_t sessionId) noexcept { sessionId_ = sessionId; }
void setSensorId(std::uint16_t sensorId) noexcept { sensorId_ = sensorId; }
void setFlagBits(std::uint8_t flagBits) noexcept { flagBits_ = flagBits; }
    */
    /**
     * @brief Serializa el mensaje a un buffer de 5 bytes.
     * Formato:
     *  [0-1]: sessionId (big endian)
     *  [2-3]: sensorId  (big endian)
     *  [4]  : flagBits
     */
    std::array<std::uint8_t, 5> toBuffer() const noexcept;

    bool operator==(const DisconnectRequest &other) const noexcept;

    bool operator!=(const DisconnectRequest &other) const noexcept;

private:
    std::uint16_t sessionId_ = 0;
    std::uint16_t sensorId_ = 0;
    std::uint8_t flagBits_ = 0;
};

/**
 * @brief Sobrecarga del operador de inserción para imprimir un DisconnectRequest.
 *
 * @param os El flujo de salida.
 * @param req El DisconnectRequest a imprimir.
 * @return std::ostream& El flujo de salida.
 */
inline std::ostream &operator<<(std::ostream &os, const DisconnectRequest &req)
{
    os << "DisconnectRequest{ sessionId=" << req.getSessionId()
       << ", sensorId=" << req.getSensorId()
       << ", flags=" << static_cast<int>(req.getFlagBits()) << " }";
    return os;
}