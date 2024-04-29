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

#include "hc05.h"

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
#define ADC_X 26
#define ADC_Y 27


void hc05_task(void *p) {
    uart_init(hc05_UART_ID, hc05_BAUD_RATE);
    gpio_set_function(hc05_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(hc05_RX_PIN, GPIO_FUNC_UART);
    hc05_init("JuliaBurno", "1234");
    gpio_init(hc05_PIN);
    gpio_set_dir(hc05_PIN, GPIO_IN);

    while(true){
        if (gpio_get(hc05_PIN) == 1) {
            //Ascender LED
            break;
        }
    }
}

/* BUTTON RELATED */
typedef struct btn{

    char ID; // button ID
    int status; // button status
} btn_t;

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
    
    xQueueSendFromISR(xQueueBtn, &btn, 0);
}

/* ADC RELATED */

typedef struct adc{
    int axis;
    int val;
}adc_t;

QueueHandle_t xQueueAdc;

#define deadZone 150

void adc_task(void *p){
    adc_init();
    adc_gpio_init(ADC_X); // X

    adc_init();
    adc_gpio_init(ADC_Y); //Y

    int zone_limit = 80;
    int mouse_speed = 20;
    
    while (1) {
        //X
        adc_select_input(0); // Select ADC input 0 (GPIO26)
        int x = adc_read();
        //printf("X: %d V\n", x);
        //Calcula a deadzone
        x = ((x-2047)/20);
        if (x <=zone_limit && x >= -1*(zone_limit)) {
            x = 0;
        }
        if (x > 0) {
            x = mouse_speed;
        }
        if (x < 0) {
            x = -mouse_speed;
        }
        struct adc adc_data_x = {0,(int)x};
        // printf("X: %d\n", x); // Debug
        if (x != 0)
            xQueueSend(xQueueAdc, &adc_data_x, 1);
        
        //Y
        adc_select_input(1); // Select ADC input 1 (GPIO27s)
        int y = adc_read();
        //printf("Y: %d V\n", y);
        //Calcula a deadzone
        y = ((y-2047)/20);
        if (y <=zone_limit && y >= -1*(zone_limit)) {
            y = 0;
        }
        if (y > 0) {
            y = mouse_speed;
        }
        if (y < 0) {
            y = -mouse_speed;
        }
        struct adc adc_data_y = {1,(int)y};
        // printf("Y: %d\n", y); // Debug

        if (y != 0)
            xQueueSend(xQueueAdc, &adc_data_y, 1);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void uart_task(void *p) {
    adc_t data1;
    btn_t data2;

    while (1) {
        if (xQueueReceive(xQueueAdc, &data1, portMAX_DELAY) == pdTRUE) {

            int val = data1.val;
            int msb = val >> 8;
            int lsb = val & 0xFF ;

            uart_putc_raw(hc05_UART_ID, 0);
            uart_putc_raw(hc05_UART_ID, data1.axis);
            uart_putc_raw(hc05_UART_ID, lsb);
            uart_putc_raw(hc05_UART_ID, msb);
            uart_putc_raw(hc05_UART_ID, -1);

/*             uart_putc_raw(uart0, 0);
            uart_putc_raw(uart0, adc.axis);
            uart_putc_raw(uart0, lsb);
            uart_putc_raw(uart0, msb);
            uart_putc_raw(uart0, -1); */
        }
        if(xQueueReceive(xQueueBtn, &data2, portMAX_DELAY) == pdTRUE){


            uart_putc_raw(hc05_UART_ID, 0);
            uart_putc_raw(hc05_UART_ID, data2.ID);
            uart_putc_raw(hc05_UART_ID, data2.status);
            uart_putc_raw(hc05_UART_ID, 0);
            uart_putc_raw(hc05_UART_ID, -1);

/*             uart_putc_raw(uart0, 1);
            uart_putc_raw(uart0, btn.ID);
            uart_putc_raw(uart0, btn.status);
            uart_putc_raw(uart0, 0);
            uart_putc_raw(uart0, -1);  */
        }
    }
}

int main() {
    stdio_init_all();
    btn_init();

    gpio_set_irq_enabled_with_callback(BTN_R, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_G, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_B, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_Y, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(JS_L, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(JS_U, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(JS_R, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(JS_D, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    xQueueBtn = xQueueCreate(32, sizeof(btn_t));
    xQueueAdc = xQueueCreate(32, sizeof(adc_t));


    xTaskCreate(hc05_task, "UART_Task 1", 4096, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);
    xTaskCreate(adc_task, "adc_task", 4096, NULL, 1, NULL);
    // xTaskCreate(adc_y_task, "adc_task2", 4096, NULL, 1, NULL);


    vTaskStartScheduler();

    while (true)
        ;
}
