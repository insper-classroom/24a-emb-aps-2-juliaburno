/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <string.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

#include "hc06.h"

#include <math.h>
#include <stdlib.h>

#define BTN_R 12
#define BTN_B 13
#define BTN_G 14
#define BTN_Y 15
#define JS_L 10
#define JS_U 11
#define JS_R 16
#define JS_D 17

typedef struct {
    char ID; // button ID
    int status; // button status
} btn_t;


QueueHandle_t xQueueBtn;

void btn_callback(uint gpio, uint32_t events) {
    btn_t btn;
    if (events == 0x4)  btn.status=1;
    else if (events == 0x8) btn.status=0; 

    if (gpio==BTN_R) {
        btn.ID='r';
    }
    if (gpio==BTN_G) {
        btn.ID='g';
    }
    if (gpio==BTN_B) {
        btn.ID='b';
    }
    if (gpio==BTN_Y) {
        btn.ID='y';
    }

    if (gpio==JS_U) {
        btn.ID='w';
    }
    if (gpio==JS_L) {
        btn.ID='a';
    }
    if (gpio==JS_D) {
        btn.ID='s';
    }
    if (gpio==JS_R) {
        btn.ID='d';
    }
    printf("botão apertado %c status-%d\n", btn.ID, btn.status);
    xQueueSendFromISR(xQueueBtn, &btn, 0);
}

void btn_task(void *p){
    gpio_init(BTN_R);
    gpio_init(BTN_B);
    gpio_init(BTN_G);
    gpio_init(BTN_Y);

    gpio_init(JS_L);
    gpio_init(JS_U);
    gpio_init(JS_R);
    gpio_init(JS_D);

    gpio_set_dir(BTN_R, GPIO_IN);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_set_dir(BTN_G, GPIO_IN);
    gpio_set_dir(BTN_Y, GPIO_IN);

    gpio_set_dir(JS_L, GPIO_IN);
    gpio_set_dir(JS_U, GPIO_IN);
    gpio_set_dir(JS_R, GPIO_IN);
    gpio_set_dir(JS_D, GPIO_IN);

    gpio_pull_up(BTN_R);
    gpio_pull_up(BTN_B);
    gpio_pull_up(BTN_G);
    gpio_pull_up(BTN_Y);
    
    gpio_pull_up(JS_L);
    gpio_pull_up(JS_U);
    gpio_pull_up(JS_R);
    gpio_pull_up(JS_D);

    gpio_set_irq_enabled_with_callback(BTN_R, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_G, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_B, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_Y, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(JS_L, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(JS_U, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(JS_R, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(JS_D, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    btn_t btn;
    while (1){
        if (xQueueReceive(xQueueBtn, &btn,pdMS_TO_TICKS(100))) {
            //printf("btn: %d %s\n",btn.status, btn.ID);
            uart_putc_raw(uart0, 1);
            uart_putc_raw(uart0, btn.ID);
            uart_putc_raw(uart0, btn.status);
            uart_putc_raw(uart0, 0);
            uart_putc_raw(uart0, -1); 
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}


int main() {
    stdio_init_all();

    xQueueBtn = xQueueCreate(32, sizeof(btn_t));
    xTaskCreate(btn_task, "btn_task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
