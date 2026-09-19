
#include "Driver_Config.hpp" 


        
USBCommDriver::USBCommDriver(const STD::string& path, int baud, int handle) : device_path(path), baud_rate(baud), device_handle(handle) {}
        
ssize_t USBCommDriver::Send(const uint8_t* data, size_t len)
{
    if (device_handle < 0)
    {
        throw std::runtime_error("Device not connected");
        return -1;
    }

    return write(device_handle, data, len);
}
ssize_t USBCommDriver::Receive(uint8_t* buffer, size_t maxLen)
{ 
    if (device_handle < 0)
    {
        throw std::runtime_error("Device not connected");
        return -1;
    }

    return read(device_handle, buffer, maxLen);
}
bool USBCommDriver::Connect()
{
    device_handle = open(device_path.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (device_handle < 0)
    {
        throw std::runtime_error("Failed to open device");
        return false;
    }
    else
    {
        struct termios tty;
        if(tcgetattr(device_handle, &tty) != 0)  //read port data and check for error
        {
            throw std::runtime_error("Failed to get terminal attributes");
            return false;
        }
        else
        {
            cfsetispeed(&tty, baud_rate); //Set input baud rate
            cfsetospeed(&tty, baud_rate); //Set output baud rate
            tty.c_cflag &= ~PARENB;  //No parity
            tty.c_cflag &= ~CSTOPB;  //1 stop bit
            tty.c_cflag &= ~CSIZE;   //Clear current char size mask
            tty.c_cflag |= CS8;      //8 bits per byte
            tty.c_cflag |= CREAD | CLOCAL;  //Turn on READ & ignore ctrl lines

            if (tcsetattr(device_handle, TCSANOW, &tty) != 0)  //Apply the new settings to the port
            {
                throw std::runtime_error("Failed to set terminal attributes");
                return false;
            }
        }
    }
    return true;
}


        
UDPCommDriver::UDPCommDriver(const std::string& ip, int port, int handle) : IP(ip), port(port), UDP_handle(handle) {}
        
ssize_t UDPCommDriver::Send(const uint8_t* data, size_t len) 
{
    if (UDP_handle < 0)
    {
        throw std::runtime_error("Socket not connected");
        return -1;
    }

    return send(UDP_handle, data, len, 0);
}
ssize_t UDPCommDriver::Receive(uint8_t* buffer, size_t maxLen) 
{
    if (UDP_handle < 0)
    {
        throw std::runtime_error("Socket not connected");
        return -1;
    }

    return recv(UDP_handle, buffer, maxLen, 0);
}
bool UDPCommDriver::Connect() 
{
    UDP_handle = socket(AF_INET, SOCK_DGRAM, 0);
    if (UDP_handle < 0)
    {
        throw std::runtime_error("Failed to create socket");
        return false;
    }
    else
    {
        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, IP.c_str(), &addr.sin_addr);

            
        return ::connect(UDP_handle, (struct sockaddr*)&addr, sizeof(addr)) == 0;

    }
}

        
TCPCommDriver::TCPCommDriver(const std::string& ip, int port, int handle) : IP(ip), port(port), socket_handle(handle) {}
        
ssize_t TCPCommDriver::Send(const uint8_t* data, size_t len) 
{
    if(socket_handle < 0)
    {
        throw std::runtime_error("Socket not connected");
        return -1;
    }

    return send(socket_handle , data, len, 0);
}

ssize_t TCPCommDriver::Receive(uint8_t * buffer, size_t maxLen) 
{
    if(socket_handle < 0)
    {
        throw std::runtime_error("Socket not connected");
        return -1;
    }
    return recv(socket_handle, buffer, maxLen, 0);
}

bool TCPCommDriver::Connect() 
{
    socket_handle = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_handle < 0)
    {
        throw std::runtime_error("Failed to create socket");
        return false;
    }
    else
    {
        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, IP.c_str(), &addr.sin_addr);

            
        return ::connect(socket_handle, (struct sockaddr*)&addr, sizeof(addr)) == 0;

    }
}

bool CommDriverConfiguration(CommDriverConfig * config)
{
    bool status = false;
    if(config->transport == TransportType::SERIAL)
    {
        USBCommDriver USB(config->device_path.value_or("/dev/ttyUSB0"), config->baud_rate.value_or(57600), -1);
        status = USB.Connect();
    }   
    else if(config->transport == TransportType::UDP)
    {
        UDPCommDriver UDP(config->host.value_or("127.0.0.1"), config->port.value_or(14550), -1);
        status = UDP.Connect();
    }
    else if(config->transport == TransportType::TCP)
    {
        TCPCommDriver TCP(config->host.value_or("127.0.0.1"), config->port.value_or(14550), -1);
        status = TCP.Connect();
    }
    else
    {
        throw std::invalid_argument("Invalid transport type");
    }
    return status;
}