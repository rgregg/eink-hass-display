#ifndef GPIO_H
#define GPIO_H

#ifdef ESP32S3

// Long Header Side
// RESET
// 3.3V
// 3.3V
// GND
#define GPIO_A0 18
#define GPIO_A1 17
#define GPIO_A2 16
#define GPIO_A3 15
#define GPIO_A4 14
#define GPIO_A5 8
#define GPIO_SCK 36
#define GPIO_MOSI 35
#define GPIO_MISO 37
// RX
// TX
// TXD0

// Short Header Side
// VBAT
// EN
// VBUS
#define GPIO_D13 13
#define GPIO_D12 12
#define GPIO_D11 11
#define GPIO_D10 10
#define GPIO_D9 9
#define GPIO_D6 6
#define GPIO_D5 5
#define GPIO_SCL 4
#define GPIO_SDA 3

#endif

#ifdef ESP32

// Long Header Side
// RESET
// 3.3V
// No Connect
// GND
#define GPIO_A0 26
#define GPIO_A1 25
#define GPIO_A2 34
#define GPIO_A3 39
#define GPIO_A4 36
#define GPIO_A5 4
#define GPIO_SCK 5
#define GPIO_MOSI 18
#define GPIO_MISO 19
#define GPIO_D16 16
#define GPIO_D17 17
#define GPIO_D21 21


// SHort Header Side
// VBAT
// EN
// VBUS
#define GPIO_D13 13
#define GPIO_D12 12
#define GPIO_D11 27
#define GPIO_D10 33
#define GPIO_D9 15
#define GPIO_D6 32
#define GPIO_D5 14
#define GPIO_SCL 22
#define GPIO_SDA 23

#endif

#endif // GPIO_H