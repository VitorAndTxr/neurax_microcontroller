/**
 * @file StreamingProtocol.h
 * @brief Binary protocol for fixed 215 Hz sEMG streaming
 *
 * FIXED CONFIGURATION (No Runtime Changes):
 * - Sampling Rate: 215 Hz (860 Hz ADC ÷ 4 downsample)
 * - Filter: Butterworth 10-50 Hz bandpass + 60 Hz notch
 * - Data Format: Binary packets (int16_t values)
 * - Packet Size: 108 bytes (header 8 + data 100)
 * - Samples/Packet: 50
 * - Packets/Sec: ~4.3
 * - Bandwidth: 464 bytes/s (48% of 9600 baud) ✅ SAFE
 *
 * Mobile App Protocol:
 *   START:  {"cd":11,"mt":"x"}  → ACK: {"cd":11,"mt":"a"}
 *   STOP:   {"cd":12,"mt":"x"}  → ACK: {"cd":12,"mt":"a"}
 *   DATA:   Binary packets (auto-sent at 215 Hz)
 *
 * NO LONGER SUPPORTED:
 *   CONFIG: {"cd":14,...} ❌ REMOVED (always 215 Hz filtered)
 *
 * Packet Structure:
 * ┌─────────────────────────────────────────────────────────┐
 * │ Header (8 bytes)                                        │
 * ├─────────────────────────────────────────────────────────┤
 * │ 0xAA | 0x0D | timestamp (4 bytes) | sample_count (2 B) │
 * ├─────────────────────────────────────────────────────────┤
 * │ Data (100 bytes for 50 samples × 2 bytes)               │
 * ├─────────────────────────────────────────────────────────┤
 * │ int16_t[0] | int16_t[1] | ... | int16_t[49]            │
 * └─────────────────────────────────────────────────────────┘
 *
 * Total packet size: 108 bytes (72% reduction vs JSON)
 */

#ifndef STREAMING_PROTOCOL_H
#define STREAMING_PROTOCOL_H

#include <stdint.h>

/**
 * @brief Magic byte for packet start detection
 */
#define PACKET_MAGIC_BYTE 0xAA

/**
 * @brief Message code for streaming data (matches JSON protocol)
 */
#define PACKET_MESSAGE_CODE_STREAM_DATA 13

/**
 * @brief Binary packet header structure (8 bytes)
 *
 * Uses __attribute__((packed)) to prevent padding/alignment issues
 * Ensures consistent binary layout across compilers
 */
struct BinaryPacketHeader {
    uint8_t  magic;         ///< Start marker: 0xAA (1 byte)
    uint8_t  message_code;  ///< Message type: 13 for STREAM_DATA (1 byte)
    uint32_t timestamp;     ///< Milliseconds since boot from millis() (4 bytes)
    uint16_t sample_count;  ///< Number of int16_t samples in payload (2 bytes)
} __attribute__((packed));

/**
 * @brief Data value range for int16_t encoding
 *
 * ADC range: 0-4.096V maps to ±4096 integer range
 * This preserves millivolt precision: 1 LSB = 1 mV
 */
#define VALUE_RANGE_MAX  4096
#define VALUE_RANGE_MIN -4096

/**
 * @brief Maximum packet size calculation
 *
 * Header (8 bytes) + Data (50 samples × 2 bytes) = 108 bytes
 */
#define MAX_BINARY_PACKET_SIZE (sizeof(BinaryPacketHeader) + (MAX_SAMPLES_PER_PACKET * sizeof(int16_t)))

#endif // STREAMING_PROTOCOL_H
