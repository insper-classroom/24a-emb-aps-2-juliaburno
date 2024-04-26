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
/* #define BTN_TEST 17 */
#define JS_L 10
#define JS_U 11
#define JS_R 16
#define JS_D 17
#define ADC_X 26
#define ADC_Y 27

typedef struct {
    int type; // 0 - adc, 1 - button
    int axis; // 0 - y, 1 - x
    int val; // value
    char ID; // button ID
    int status; // button status
} command;

void hc06_task(void *p) {
    uart_init(HC06_UART_ID, HC06_BAUD_RATE);
    gpio_set_function(HC06_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(HC06_RX_PIN, GPIO_FUNC_UART);
    hc06_init("aps2_julia_burno", "1234");

/*     while (true) {
        uart_puts(HC06_UART_ID, "OLAAA ");
        vTaskDelay(pdMS_TO_TICKS(100));
    } */
}


void write_package(command data) {
    if (data.type == 0) { // adc
        int val = data.val;
        int msb = val >> 8;
        int lsb = val & 0xFF ;

        uart_putc_raw(uart0, data.type);
        uart_putc_raw(uart0, data.axis);
        uart_putc_raw(uart0, lsb);
        uart_putc_raw(uart0, msb);
        uart_putc_raw(uart0, -1);
        return;
    }
    else if (data.type == 1) { // button
        uart_putc_raw(uart0, data.type);
        uart_putc_raw(uart0, data.ID);
        uart_putc_raw(uart0, data.status);
        uart_putc_raw(uart0, -1);
        uart_putc_raw(uart0, -1);
        return;
    }
}


/* BUTTON RELATED */

void btn_init(){
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

    
}

QueueHandle_t xQueueBtn;


void btn_callback(uint gpio, uint32_t events) {
    command btn;
    btn.type=1;
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
    
    xQueueSendFromISR(xQueueBtn, &btn, 0);
}

void btn_task(void *p){
    btn_init();
    //printf("BTN_INIT");

    command btn;
    while (1){
        if (xQueueReceive(xQueueBtn, &btn,pdMS_TO_TICKS(100))) {
            //printf("btn: %d %s\n",btn.status, btn.ID);
            write_package(btn); 
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}



/* ADC RELATED */

QueueHandle_t xQueueAdc;

#define deadZone 150


void adc_x_task(void *p) {
    command data;
    data.type = 0;
    adc_init();
    adc_gpio_init(26);

    while (1) {
        adc_select_input(0); // Select ADC input 0 (GPIO26)
        float result = adc_read();

        result = result - 2048;
        result = result / 8;

        if (abs(result) < deadZone) {
            result = 0;
        }

        data.val = result;
        data.axis = 1;
        xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void adc_y_task(void *p) {
    command data;
    data.type = 0;
    adc_init();
    adc_gpio_init(27);

    while (1) {
        adc_select_input(1); // Select ADC input 1 (GPIO27)
        float result = adc_read();

        result = result - 2048;
        result = result / 8;

        if (abs(result) < deadZone) {
            result = 0;
        }
        data.val = result;
        data.axis = 0;
        xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void uart_task(void *p) {
    command data;

    while (1) {
        if (xQueueReceive(xQueueAdc, &data, portMAX_DELAY) == pdTRUE) {
            write_package(data);
        }
    }
}


int main() {
    stdio_init_all();

    gpio_set_irq_enabled_with_callback(BTN_R, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_G, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_B, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_Y, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(JS_L, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(JS_U, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(JS_R, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(JS_D, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    

    printf("Start bluetooth task\n");

    xTaskCreate(hc06_task, "UART_Task 1", 4096, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);

    xQueueAdc = xQueueCreate(32, sizeof(command));
    xTaskCreate(adc_x_task, "adc_task", 4096, NULL, 1, NULL);
    xTaskCreate(adc_y_task, "adc_task2", 4096, NULL, 1, NULL);

    xQueueBtn = xQueueCreate(32, sizeof(command));
    xTaskCreate(btn_task, "btn_task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
