/*
 * clunet.c
 *
 * Created: 25.08.2019 15:31:16
 *  Author: Dmitry
 */ 

/* Name: clunet.c
 * Project: CLUNET bus driver
 * Author: Alexey Avdyukhin
 * Creation Date: 2013-09-09
 * License: DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 */
#define F_CPU 1000000

#include "clunet_config.h"
#include "bits.h"
#include "clunet.h"

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

void (*on_data_received)(uint8_t src_address, uint8_t dst_address, uint8_t command, char* data, uint8_t size) = 0;
void (*on_data_received_sniff)(uint8_t src_address, uint8_t dst_address, uint8_t command, char* data, uint8_t size) = 0;

volatile uint8_t clunet_sending_state = CLUNET_SENDING_STATE_IDLE;
volatile uint8_t clunet_sending_data_length;
volatile uint8_t clunet_sending_current_byte;
volatile uint8_t clunet_sending_current_bit;
volatile uint8_t clunet_reading_state = CLUNET_READING_STATE_IDLE;
volatile uint8_t clunet_reading_current_byte;
volatile uint8_t clunet_reading_current_bit;
volatile uint8_t clunet_sending_priority;
volatile uint8_t clunetTimerStart = 0;
volatile char out_buffer[CLUNET_SEND_BUFFER_SIZE];
volatile char in_buffer[CLUNET_READ_BUFFER_SIZE];

static inline void
clunet_start_send()
{
    clunet_sending_state = CLUNET_SENDING_STATE_INIT;
    // �������� 1.5�, ����� ��� �������������� ����� ���������� ��� �������� �� ����� �� ������� ������� ���������� � ��������� �������� ����������
    CLUNET_TIMER_REG_OCR = CLUNET_TIMER_REG + (CLUNET_T + CLUNET_T / 2);
    CLUNET_ENABLE_TIMER_COMP; // �������� ���������� ��������� ������� (��������)
}

static inline char
check_crc(const char* data, const uint8_t size)
{
      uint8_t crc = 0;
      uint8_t i, j;
      for (i = 0; i < size; i++)
      {
            uint8_t inbyte = data[i];
            for (j = 0 ; j < 8 ; j++)
            {
                  uint8_t mix = (crc ^ inbyte) & 1;
                  crc >>= 1;
                  if (mix) crc ^= 0x8C;
                  inbyte >>= 1;
            }
      }
      return crc;
}

static inline void
clunet_data_received(const uint8_t src_address, const uint8_t dst_address, const uint8_t command, char* data, const uint8_t size)
{
    if (on_data_received_sniff)
        (*on_data_received_sniff)(src_address, dst_address, command, data, size);

    if (src_address == CLUNET_DEVICE_ID) return; // ���������� ��������� �� ������ ����!

    if ((dst_address != CLUNET_DEVICE_ID) &&
        (dst_address != CLUNET_BROADCAST_ADDRESS)) return; // ���������� ��������� �� ��� ���

    // ������� ������������. ������������ �� ����������� �������
    if (command == CLUNET_COMMAND_REBOOT)
    {
        cli();
        set_bit(WDTCR, WDE);
        while(1);
    }

    if ((clunet_sending_state == CLUNET_SENDING_STATE_IDLE) || (clunet_sending_priority <= CLUNET_PRIORITY_MESSAGE))
    {
        /* ����� �� ����� ��������� */
        if (command == CLUNET_COMMAND_DISCOVERY)
        {
#ifdef CLUNET_DEVICE_NAME
            char buf[] = CLUNET_DEVICE_NAME;
            uint8_t len = 0; while(buf[len]) len++;
            clunet_send(src_address, CLUNET_PRIORITY_MESSAGE, CLUNET_COMMAND_DISCOVERY_RESPONSE, buf, len);
#else
            clunet_send(src_address, CLUNET_PRIORITY_MESSAGE, CLUNET_COMMAND_DISCOVERY_RESPONSE, 0, 0);
#endif
        }
        /* ����� �� ���� */
        else if (command == CLUNET_COMMAND_PING)
            clunet_send(src_address, CLUNET_PRIORITY_COMMAND, CLUNET_COMMAND_PING_REPLY, data, size);
    }

    if (on_data_received)
        (*on_data_received)(src_address, dst_address, command, data, size);
}

/* ��������� ���������� ��������� ������� */
ISR(CLUNET_TIMER_COMP_VECTOR)
{
    /* ���� �������� ���� ���������� ��������, �� �������� �� � ��������� ���������� */
    if (clunet_sending_state == CLUNET_SENDING_STATE_DONE)
    {
        CLUNET_DISABLE_TIMER_COMP;                // ��������� ���������� ��������� �������
        clunet_sending_state = CLUNET_SENDING_STATE_IDLE;        // ���������, ��� ���������� ��������
        CLUNET_SEND_0;                        // ��������� �����
    }
    /* ����� ���� �������� ���������� ����������, �� ������� �������� �� �������� */
    else if (!CLUNET_SENDING && CLUNET_READING)
    {
        CLUNET_DISABLE_TIMER_COMP;                    // ��������� ���������� ��������� ������� (��������)
        clunet_sending_state = CLUNET_SENDING_STATE_WAITING_LINE;        // ��������� � ����� �������� �����
    }
    /* ��� � �������, ����� ���������� */
    else
    {
        CLUNET_SEND_INVERT;    // ����������� �������� �������
        
        /* ���� ��������� �����, �� ����������� ����� ����� ����� ��������� ��������� ������������� 1� */
        if (!CLUNET_SENDING)
            CLUNET_TIMER_REG_OCR += CLUNET_T;
    
        /* ���� ������� ����� � �����, �� ����������� ����� �������� ������� � ����������� �� ������� ���� �������� */
        /* ���� �������� ������ */
        else if (clunet_sending_state == CLUNET_SENDING_STATE_DATA)
        {
            /* ��������� ��������� ���������� � ����������� �� �������� ���� */
            CLUNET_TIMER_REG_OCR += ((out_buffer[clunet_sending_current_byte] & (1 << clunet_sending_current_bit)) ? CLUNET_1_T : CLUNET_0_T);
            /* ���� ������� ���� ������ */
            if (++clunet_sending_current_bit & 8)
            {
                /* ���� �� ��� ������ �������� */
                if (++clunet_sending_current_byte < clunet_sending_data_length)
                    clunet_sending_current_bit = 0; // �������� �������� ���������� ����� � ���� 0
                /* ����� �������� ���� ������ ��������� */
                else
                    clunet_sending_state++; // ��������� � ��������� ���� ���������� �������� ������
            }
        }
        else
            switch (clunet_sending_state++)
            {
            /* ���� ������������� �������� ������ (����� 10�) */
            case CLUNET_SENDING_STATE_INIT:
                CLUNET_TIMER_REG_OCR += CLUNET_INIT_T;
                break;
            /* ���� �������� ���������� (������� ���) */
            case CLUNET_SENDING_STATE_PRIO1:
                CLUNET_TIMER_REG_OCR += ((clunet_sending_priority > 2) ? CLUNET_1_T : CLUNET_0_T);
                break;
            /* ���� �������� ���������� (������� ���) */
            case CLUNET_SENDING_STATE_PRIO2:
                CLUNET_TIMER_REG_OCR += ((clunet_sending_priority & 1) ? CLUNET_0_T : CLUNET_1_T);
                clunet_sending_current_byte = clunet_sending_current_bit = 0;    // ������� �������� �������� ������
            }
    }
}

void
clunet_send(const uint8_t address, const uint8_t prio, const uint8_t command, const char* data, const uint8_t size)
{
    /* ���� ������ ������ � �������� ������ �������� (����������� ��� ��������� 250 ����) */
    if (size < (CLUNET_SEND_BUFFER_SIZE - CLUNET_OFFSET_DATA))
    {
        /* ��������� ������� ��������, ���� ���� ����� */
        if (clunet_sending_state)
        {
            CLUNET_DISABLE_TIMER_COMP;
            CLUNET_SEND_0;
        }

        /* ��������� ���������� */
        clunet_sending_priority = (prio > 4) ? 4 : prio ? : 1;    // ��������� ��������� ���������� (1 ; 4)
        out_buffer[CLUNET_OFFSET_SRC_ADDRESS] = CLUNET_DEVICE_ID;
        out_buffer[CLUNET_OFFSET_DST_ADDRESS] = address;
        out_buffer[CLUNET_OFFSET_COMMAND] = command;
        out_buffer[CLUNET_OFFSET_SIZE] = size;
        
        /* �������� ������ � ����� */
        uint8_t i;
        for (i = 0; i < size; i++)
            out_buffer[CLUNET_OFFSET_DATA + i] = data[i];

        /* ��������� ����������� ����� */
        out_buffer[CLUNET_OFFSET_DATA + size] = check_crc((char*)out_buffer, CLUNET_OFFSET_DATA + size);
        
        clunet_sending_data_length = size + (CLUNET_OFFSET_DATA + 1);

        // ���� ����� ��������, �� ����������� �������� �����
        if (!CLUNET_READING)
            clunet_start_send();
        // ����� ����� ������� ����� ����������� � ��������� �������� ����������
        else
            clunet_sending_state = CLUNET_SENDING_STATE_WAITING_LINE;
    }
}

/* ��������� �������� ���������� �� ������ � ����� ������� */
ISR(CLUNET_INT_VECTOR)
{
    uint8_t now = CLUNET_TIMER_REG;        // ������� �������� �������

    /* ���� ����� ������� � ���� */
    if (CLUNET_READING)
    {
        clunetTimerStart = now;        // �������� ����� ������ �������
        /* ���� �� � ������ �������� � ������� �� ��, �� ��������� � �������, ��� �����, ��� ���� ������������ ������ ��� ����� */
        /* �������������� ������� ��������� ������ �� ����� �� ��������� ���������� */
        if (clunet_sending_state && !CLUNET_SENDING)
        {
            CLUNET_DISABLE_TIMER_COMP;
            clunet_sending_state = CLUNET_SENDING_STATE_WAITING_LINE;
        }
    }
    /* ����� ���� ����� ��������� */
    else
    {
        /* ����� ��������, ������� ������������� �������� */
        if (clunet_sending_state == CLUNET_SENDING_STATE_WAITING_LINE)
            clunet_start_send();

        uint8_t ticks = now - clunetTimerStart;    // �������� ����� ������� � ����� �������

        /* ���� ���-�� ����� ��� ����� (����� >= 6.5�) - ��� ������������� */
        if (ticks >= (CLUNET_INIT_T + CLUNET_1_T) / 2)
        {
            clunet_reading_state = CLUNET_READING_STATE_PRIO1;
        }
        /* ����� ���� �������, �� ������� �� ���� */
        else
            switch (clunet_reading_state)
            {
            
            /* ������ ������ */
            case CLUNET_READING_STATE_DATA:

                /* ���� ��� �������� (����� > 2�), �� ��������� ��� � �������� ������ */
                if (ticks > (CLUNET_0_T + CLUNET_1_T) / 2)
                    in_buffer[clunet_reading_current_byte] |= (1 << clunet_reading_current_bit);

                /* �������������� ��������� ����, � ��� ������ ��������� ���� 8 ��� � ����� ��������: */
                if (++clunet_reading_current_bit & 8)
                {

                    /* �������� �� ��������� ������ ������ */
                    if ((++clunet_reading_current_byte > CLUNET_OFFSET_SIZE) && (clunet_reading_current_byte > in_buffer[CLUNET_OFFSET_SIZE] + CLUNET_OFFSET_DATA))
                    {

                        clunet_reading_state = CLUNET_READING_STATE_IDLE;

                        /* ��������� CRC, ��� ������ ������ ��������� ��������� ������ */
                        if (!check_crc((char*)in_buffer, clunet_reading_current_byte))
                            clunet_data_received (
                                in_buffer[CLUNET_OFFSET_SRC_ADDRESS],
                                in_buffer[CLUNET_OFFSET_DST_ADDRESS],
                                in_buffer[CLUNET_OFFSET_COMMAND],
                                (char*)(in_buffer + CLUNET_OFFSET_DATA),
                                in_buffer[CLUNET_OFFSET_SIZE]
                            );
                    
                    }
                    
                    /* ����� ���� ����� �� �������� � ����� �� ���������� - ������������ � ������ ���������� ����� */
                    else if (clunet_reading_current_byte < CLUNET_READ_BUFFER_SIZE)
                    {
                        clunet_reading_current_bit = 0;
                        in_buffer[clunet_reading_current_byte] = 0;
                    }
                    
                    /* ����� - �������� ��������� ������ -> ���������� ����� */
                    else
                        clunet_reading_state = CLUNET_READING_STATE_IDLE;

                }
                break;

            /* ��������� ���������� (������� ���), ������� �� �� ����� */
            case CLUNET_READING_STATE_PRIO2:
                clunet_reading_current_byte = clunet_reading_current_bit = 0;
                in_buffer[0] = 0;

            /* ��������� ���������� (������� ���), ������� �� �� ����� */
            case CLUNET_READING_STATE_PRIO1:
                clunet_reading_state++;
                break;
            }
    }
}

void
clunet_init()
{
    sei();
    CLUNET_SEND_INIT;
    CLUNET_READ_INIT;
    CLUNET_TIMER_INIT;
    CLUNET_INIT_INT;
    char reset_source = MCUCSR;
    clunet_send (
        CLUNET_BROADCAST_ADDRESS,
        CLUNET_PRIORITY_MESSAGE,
        CLUNET_COMMAND_BOOT_COMPLETED,
        &reset_source,
        sizeof(reset_source)
    );
    MCUCSR = 0;
}

/* ���������� 0, ���� ����� � ��������, ����� ��������� ������� ������ */
uint8_t
clunet_ready_to_send()
{
    return clunet_sending_state ? clunet_sending_priority : 0;
}

void
clunet_set_on_data_received(void (*f)(uint8_t src_address, uint8_t dst_address, uint8_t command, char* data, uint8_t size))
{
    on_data_received = f;
}

void
clunet_set_on_data_received_sniff(void (*f)(uint8_t src_address, uint8_t dst_address, uint8_t command, char* data, uint8_t size))
{
    on_data_received_sniff = f;
}