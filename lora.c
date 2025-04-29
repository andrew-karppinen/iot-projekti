#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "project.h"
// read responses from lora
static bool read_response(char *buffer, size_t max_length, uint32_t timeout_ms) {
    size_t idx = 0;
    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);

    while (!time_reached(deadline)) {
        if (uart_is_readable(UART_ID)) {
            char c = uart_getc(UART_ID);
            if (c == '\n') {
                buffer[idx] = '\0';
                return true;
            }
            if (idx < max_length - 1) {
                buffer[idx++] = c;
            }
        }
    }
    buffer[0] = '\0';
    return false;
}
//setup lora
bool set_up_lora(void) {
    for (int i = 0; i < 2; i++) {
        uart_init(UART_ID, BAUD_RATE);
        gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
        gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

        char resp[150];

        uart_puts(UART_ID, "AT\r\n");
        if (!read_response(resp, sizeof(resp), 2000) || !strstr(resp, "OK")) {
            continue;
        }

        uart_puts(UART_ID, "AT+MODE=LWOTAA\r\n");
        if (!read_response(resp, sizeof(resp), 5000)) continue;

        uart_puts(UART_ID, "AT+KEY=APPKEY,\"585b2d2ae83096eb49df206a0e47d774\"\r\n");// vaihda omaan appkey
        if (!read_response(resp, sizeof(resp), 5000)) continue;

        uart_puts(UART_ID, "AT+CLASS=A\r\n");
        if (!read_response(resp, sizeof(resp), 5000)) continue;

        uart_puts(UART_ID, "AT+PORT=8\r\n");
        if (!read_response(resp, sizeof(resp), 5000)) continue;

        uart_puts(UART_ID, "AT+JOIN\r\n");
        absolute_time_t deadline = make_timeout_time_ms(20000);
        while (!time_reached(deadline)) {
            if (read_response(resp, sizeof(resp), 1000)) {
                printf("JOIN response: %s\n", resp);
                if (strstr(resp, "+JOIN: Done")) {
                    return true;
                }
                if (strstr(resp, "Join failed") || strstr(resp, "+JOIN: FAIL")) {
                    break;
                }
            }
        }
    }
    return false;
}

//sendmessages to lora
//odottaa 'DONE' kuittausta ennen kuin lähettää seuraavan viestin
void sen_lora_msg(const char *msg) {
    char cmd[200];
    char resp[200];

    // Lähetä AT+MSG
    snprintf(cmd, sizeof(cmd), "AT+MSG=\"%s\"\r\n", msg);
    uart_puts(UART_ID, cmd);
    printf("Message sent: %s\n", msg);

    // Rauhoitutaan viestien lähettämisen kanssa
    // Odotetaan 10 sekuntia jotta saadaan DONE eli viesti lähetys ok
    absolute_time_t deadline = make_timeout_time_ms(10000);
    while (!time_reached(deadline)) {
        if (read_response(resp, sizeof(resp), 1000)) {
            printf("Response: %s\n", resp);

            //onnistuu:
            if (strstr(resp, "+MSG: Done")) {
                return;
            }
            // ei onnistu:
            if (strstr(resp, "ERROR") || strstr(resp, "FAIL") || strstr(resp, "busy")) {
                printf("Lora send failure: %s\n", resp);
                return;
            }
        }
    }
    printf("Ei saatu 'DONE' \"%s\"\n", msg);
}
