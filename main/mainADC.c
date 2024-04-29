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


#define ADC_X 26
#define ADC_Y 27

typedef struct adc{
    int axis;
    int val;
}adc_t;

QueueHandle_t xQueueAdc;

#define deadZone 150

void uart_task(void *p) {
    adc_t data;

    while (1) {
        if (xQueueReceive(xQueueAdc, &data, portMAX_DELAY) == pdTRUE) {

            int val = data.val;
            int msb = val >> 8;
            int lsb = val & 0xFF ;

            uart_putc_raw(uart0, 0);
            uart_putc_raw(uart0, data.axis);
            uart_putc_raw(uart0, lsb);
            uart_putc_raw(uart0, msb);
            uart_putc_raw(uart0, -1);
        }
    }
}

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

int main() {
    stdio_init_all();
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);

    xQueueAdc = xQueueCreate(32, sizeof(adc_t));
    xTaskCreate(adc_task, "adc_task", 4096, NULL, 1, NULL);



    vTaskStartScheduler();

    while (true)
        ;
}
