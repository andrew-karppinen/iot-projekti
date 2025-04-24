#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "project.h"

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


void init_lora(void) {
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
}

bool ping_lora(void) {
    char resp[64];
    uart_puts(UART_ID, "AT\r\n");
    if (read_response(resp, sizeof(resp), 2000) && strstr(resp, "OK")) {
        return true;
    }
    return false;
}

bool configure_lora(void) {
    char resp[128];

    uart_puts(UART_ID, "AT+MODE=LWOTAA\r\n");
    if (!read_response(resp, sizeof(resp), 5000)) return false;

    uart_puts(UART_ID, "AT+KEY=APPKEY,\"585b2d2ae83096eb49df206a0e47d774\"\r\n"); //vaihda oma appkey
    if (!read_response(resp, sizeof(resp), 5000)) return false;

    uart_puts(UART_ID, "AT+CLASS=A\r\n");
    if (!read_response(resp, sizeof(resp), 5000)) return false;

    uart_puts(UART_ID, "AT+PORT=8\r\n");
    if (!read_response(resp, sizeof(resp), 5000)) return false;

    return true;
}

bool join_lora(void) {
    char resp[128];
    uart_puts(UART_ID, "AT+JOIN\r\n");

    absolute_time_t deadline = make_timeout_time_ms(20000);
    while (!time_reached(deadline)) {
        if (read_response(resp, sizeof(resp), 1000)) {
            // debug-tulostus
            printf("JOIN response: %s\n", resp);
            if (strstr(resp, "+JOIN: Done")) {
                printf("Joined Lora. Calibration can start press sw\n");
                return true;
            }
            if (strstr(resp, "Join failed") || strstr(resp, "+JOIN: FAIL")) {
                return false;
            }
        }
    }
    //timeout
    return false;
}

void sen_lora_msg(const char *msg) {
    char cmd[160];
    char resp[128];

    snprintf(cmd, sizeof(cmd), "AT+MSG=\"%s\"\r\n", msg);
    uart_puts(UART_ID, cmd);

    // debug
    printf("Message sent: %s\n", msg);

    if (read_response(resp, sizeof(resp), 10000)) {
        printf("Response: %s\n", resp);
    } else {
        printf("No response to message\n");
    }
}
