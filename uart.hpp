#ifndef __UART_HPP
#define __UART_HPP

// 标准库


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

private:
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

