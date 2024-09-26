#ifndef __UART_HPP
#define __UART_HPP

// 标准库
#include <iostream>
#include <map>

// 第三方库
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

class Uart {
public:
    /**
     * @brief 构造函数
     * @param port      : 串口设备路径
     * @param baudRate  : 波特率，默认为9600
     * @param hfc       : 是否启用硬件流控制，默认不启用
     * @param sfc       : 是否启用软件流控制，默认不启用
     * @param _stopBits : 停止位数，默认为1
     * @param _dataBits : 数据位数，默认为8
     * @param _parity   : 是否启用奇偶校验，默认不启用。'N'表示无校验，'E'表示偶校验，'O'表示奇校验
     * @note 构造函数中完成所有属性的初始化，但是不会应用设置的属性（即不会打开串口）
     */
    Uart(const char* port, speed_t baudRate = 9600, bool hfc = false, bool sfc = false, char parity = 'N', int stopBits =1 , int dataBits = 8)
        : _port(port)
        , _baudRate(baudRate)
        , _fd(-1) 
        , _hfc(hfc)
        , _sfc(sfc)
        , _parity(parity)
        , _stopBits(stopBits)
        , _dataBits(dataBits)
        , _open(false) {
        } /* Uart(const char* port, int baudRate) { */
    
    ~Uart() {

    }

    /**
     * @brief 打开串口
     * @return 状态，true串口打开成功，反之表示串口打开失败
     */
    bool open() {

        if (!configure()) {
            try {
                close();
            } catch (std::runtime_error& e) {
                std::cerr << e.what() << std::endl;
            }
            //close();
            return false;
        }

        // 应用配置
        try {
            setAttributes();
        } catch (std::runtime_error& e) {
            std::cerr << e.what() << std::endl;
        }

        // 打开串口的步骤：
        // 1.调用open()系统调用打开设备文件
        // 2.配置串口各种参数，并应用更改
        // 只有当这两个步骤都成功后，串口才算打开成功
        // 如果对象调用API修改配置，则串口自动被关闭
        _open = true;

        return true;
    } /* bool open() {*/

    /**
     * @brief 关闭串口
     */
    void close() {
        _open = false;
        if (_fd != -1) {

            if (::close(_fd) == -1) {
                // std::cerr << "Error closing UART port" << std::endl;
                throw std::runtime_error("Error in closing UART port.");
            }

            _fd = -1;
        }
    } /* void close() { */


    /**
     * @brief 配置波特率
     * @param baudRate : 波特率，直接传入实际大小，而非termios定义的位图
     * @note 一旦修改配置，串口将自动关闭，需要重新打开串口
     */
    void configBaudRate(speed_t baudRate) {
        _baudRate = baudRate;
        _open     = false;

        // 输入值和位图之间的映射
        static const std::map<speed_t, speed_t> baudRateMap = {
            {      0,       B0}, {     50,      B50}, {     75,      B75},
            {    110,     B110}, {    134,     B134}, {    150,     B150},
            {    200,     B200}, {    300,     B300}, {    600,     B600},
            {   1200,    B1200}, {   1800,    B1800}, {   2400,    B2400},
            {   4800,    B4800}, {   9600,    B9600}, {  19200,   B19200},
            {  38400,   B38400}, {  57600,   B57600}, { 115200,  B115200},
            { 230400,  B230400}, { 460800,  B460800}, { 500000,  B500000},
            { 576000,  B576000}, { 921600,  B921600}, {1000000, B1000000},
            {1152000, B1152000}, {1500000, B1500000}, {2000000, B2000000},
            {2500000, B2500000}, {3000000, B3000000}, {3500000, B3500000},
            {4000000, B4000000}
        };

        auto item = baudRateMap.find(_baudRate);
        
        if (item == baudRateMap.end()) {
            throw std::invalid_argument("Invalid baud rate config");
        }

        // 这两个API本质上仍然是在操作_tty结构体，并未应用更改
        cfsetispeed(&_tty, item->second);
        cfsetospeed(&_tty, item->second);
    } /* void configBaudRate(int baudRate) { */

    /**
     * @brief 设置数据位的长度
     * @param dataBits : 数据位的长度（5，6，7，8）
     * @note 一旦修改配置，串口将自动关闭，需要重新打开串口
     */
    void configDataBits(int dataBits) {
        _dataBits     = dataBits;
        _open         = false;
        _tty.c_cflag &= ~CSIZE; // 清除旧的数据位设置

        switch (dataBits) {
            case 5:
                _tty.c_cflag |= CS5;
                break;
            case 6:
                _tty.c_cflag |= CS6;
                break;
            case 7:
                _tty.c_cflag |= CS7;
                break;
            case 8:
                _tty.c_cflag |= CS8;
                break;
            default:
                throw std::invalid_argument("Invalid data bits config.");
        }
        // tcsetattr(_fd, TCSANOW, &_tty);
        // setAttributes(_tty);
    } /* void configDataBits(int dataBits) { */

    /**
     * @brief 设置奇偶校验为
     * @param parity : 奇偶校验类型
     * @note 一旦修改配置，串口将自动关闭，需要重新打开串口
     */
    void configParity(char parity) {
        _parity = parity;
        _open   = false;

        switch (parity) {
            case 'N': // 无校验
                _tty.c_cflag &= ~PARENB;
                break;
            case 'E': // 偶校验
                _tty.c_cflag |= PARENB; // 开启奇偶校验
                _tty.c_cflag &= ~PARODD; // 偶校验
                break;
            case 'O': // 奇校验
                _tty.c_cflag |= PARENB;
                _tty.c_cflag &= PARODD;
                break;
            default:
                throw std::invalid_argument("Invalid parity config.");
                break;
        } /* switch (parity) { */
        // tcsetattr(_fd, TCSANOW, &_tty);
        // setAttributes(_tty);
    } /* void configParity(char parity) { */

    /** 
     * @brief 配置停止位
     * @param stopBits : 停止位数（1或者2）
     * @note 一旦修改配置，串口将自动关闭，需要重新打开串口
     */
    void configStopBits(int stopBits) {
        _stopBits = stopBits;
        _open     = false;

        if (stopBits == 1) {
            _tty.c_cflag &= ~CSTOPB;
        } else if (stopBits == 2) {
            _tty.c_cflag |= CSTOPB;
        } else {
            throw std::invalid_argument("Invalid stop bits config.");
        }

        // tcsetattr(_fd, TCSANOW, &_tty);
        // setAttributes(_tty);
    } /* void configStopBits(int stopBits) {*/

    /**
     * @brief 配置硬件流控制
     * @param enable : 是否启用硬件流控制
     * @param 一旦修改配置，串口将自动关闭，需要重新打开串口
     */
    void configHardwareFlowControl(bool enable) {
        _hfc  = enable;
        _open = false;

        if (enable) {
            _tty.c_cflag |= CRTSCTS;
        } else {
            _tty.c_cflag &= ~CRTSCTS;
        } /* if (enable) { */

        // tcsetattr(_fd, TCSANOW, &_tty);
        // setAttributes(_tty);
    } /* configHardwareFlowControl(bool state) { */

    /**
     * @brief 设置软件流控制
     * @param enable : 是否启用软件流控制
     */
    void configSoftwareFlowControl(bool enable) {
        _sfc  = enable;
        _open = false;
        
        if (enable) {
            _tty.c_iflag |= (IXON | IXOFF | IXANY);
        } else {
            _tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        } /* if (state == true) { */

        // tcsetattr(_fd, TCSANOW, &_tty);
        // setAttributes(_tty);
    } /* void configSoftwareFlowControl(bool state) { */

    /**
     * @brief 应用配置
     * @note 串口的所有配置应该写入_tty结构体中，然后再调佣此API进行应用
     *       此API不会打开串口，调用完成后，需要调用open()打开串口
     */
    void setAttributes() {
        _open = false;

        if (tcsetattr(_fd, TCSANOW, &_tty) ==  -1) {
            throw std::runtime_error("Error in settring attributes.");
        }

    }

    /**
     * @brief 获取串口设备路径
     */
    const char* getPort() const {
        return _port;
    }

    /**
     * @brief 获取波特率
     */
    int getBaudRate() const {
        return _baudRate;
    }

    /**
     * @brief 获取当前设备的文件描述符
     * @return 返回文件描述符
     */
    int getFd() const {
        return _fd;
    } /* int getFd() const { */

    /**
     * @brief 获取硬件流控制状态
     * @return true表示开启硬件流控制，反之表示关闭硬件流控制
     */
    bool getHfcState() const {
        return _hfc;
    }

    /**
     * @brief 获取软件流控制状态
     * @return true表示开启软件流控制，反之表示关闭软件流控制
     */
    bool getSfcState() const {
        return _sfc;
    }

    /**
     * @brief 获取停止位数
     * @return 返回停止位数
     */
    int getStopBits() const {
        return _stopBits;
    }

    /**
     * @brief 检查串口是否已经打开
     * @return true表示串口已经打开，反之表示串口未打开
     */
    bool isOpen() const {
        return _open;
    } /* bool isOpen() const { */
    
    /**
     * @brief 获取串口属性
     * @return 返回串口属性结构体
     */
    struct termios getAttributes() const {
        struct termios tty;

        if (tcgetattr(_fd, &tty) ==  -1) {
            throw std::runtime_error("Error in getting attributes.");
        } /* if (tcgetattr(_fd, &tty) == -1) { */

        return tty;
    } /* struct termios getAttributs() const { */

private:
    /**
     * @brief 配置串口
     */
    bool configure() {

        try {
            configBaudRate(_baudRate);
            configParity(_parity); // 无奇偶校验
            configStopBits(_stopBits); // 1个停止位
            configDataBits(_dataBits); // 8个数据位
            configHardwareFlowControl(_hfc); // 无硬件流控制
            configSoftwareFlowControl(_sfc); // 无软件流控制
        } catch (std::invalid_argument& e) {
            std::cerr << e.what() << std::endl;
            return false;
        } catch (std::runtime_error& e) {
            std::cerr << e.what() << std::endl;
            return false;
        }
        
        return true;
    }

    const char* _port;   // 设备路径
    speed_t _baudRate;   // 波特率
    bool _hfc;           // 是否启用硬件流控制
    bool _sfc;           // 是否启用软件流控制
    char _parity;        // 是否启用奇偶校验
    int _stopBits;       // 停止位数
    int _dataBits;       // 数据位数

    int _fd;             // tty设备的文件描述符
    struct termios _tty; // tty设备的配置信息
    bool _open;          // 串口是否已经打开
};

#endif /* __UART_HPP */

