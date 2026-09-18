#ifndef DRIVER_CONFIG_HPP
#define DRIVER_CONFIG_HPP

#include <string>
#include <optional>
#include <cstdint>   // for uint8_t
#include <cstddef>   // for size_t
#include <sys/types.h>

enum class TransportType {
    SERIAL,
    UDP,
    TCP
};

enum class MavlinkVersion {
    V1,
    V2
};

class ICommDriver {
public:
    virtual bool Connect() = 0;
    virtual ssize_t Send(const uint8_t* data, size_t len) = 0;
    virtual ssize_t Receive(uint8_t* buffer, size_t maxLen) = 0;
    virtual ~ICommDriver() {};
};


class USBCommDriver : public ICommDriver {
        std::string device_path;
        int baud_rate;
        int device_handle;
    public:
        USBCommDriver(const std::string& path, int baud, int handle) : device_path(path), baud_rate(baud), device_handle(handle) {}
        ssize_t Send(const uint8_t* data, size_t len) override;
        ssize_t Receive(uint8_t* buffer, size_t maxLen) override;
        bool Connect() override;
};

class UDPCommDriver : public ICommDriver {
        std::string IP;
        int port;
        int UDP_handle;
    public: 
        UDPCommDriver(const std::string& ip, int port, int handle) : IP(ip), port(port), UDP_handle(handle) {}
        ssize_t Send(const uint8_t* data, size_t len) override;
        ssize_t Receive(uint8_t* buffer, size_t maxLen) override;
        bool Connect() override;
};

class TCPCommDriver : public ICommDriver {
        std::string IP;
        int port;
        int socket_handle;
    public:
        TCPCommDriver(const std::string& ip, int port, int handle) : IP(ip), port(port), socket_handle(handle) {}
        ssize_t Send(const uint8_t* data, size_t len) override;
        ssize_t Receive(uint8_t* buffer, size_t maxLen) override;
        bool Connect() override;
};


struct CommDriverConfig {
    // Transport (unchanged from before)
    TransportType transport;
    std::optional<std::string> device_path;
    std::optional<int> baud_rate;
    std::optional<std::string> host;
    std::optional<int> port;

    // MAVLink identity — new
    uint8_t system_id;
    uint8_t component_id;
    uint8_t target_system_id;
    uint8_t target_component_id;

    // Protocol behavior — new
    MavlinkVersion protocol_version;
    bool use_signing;
    std::optional<std::string> signing_key;
    double own_heartbeat_rate_hz;
    int heartbeat_timeout_ms;

    // Parsing — new
    size_t receive_buffer_size;
    int max_reassembly_attempts;

    // Lifecycle (unchanged from before)
    int connect_timeout_ms;
    int max_retries;
};

extern bool CommDriverConfiguration(CommDriverConfig * config);

#endif // DRIVER_CONFIG_HPP