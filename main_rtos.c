/*
 * main_rtos.c
 *
 *  Created on: Jun 24, 2016
 *      Author: jon
 */


/* Standard includes. */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* Platform includes. */
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

/* LZ4 compression includes */
#include "lz4.h"

enum {
    MESSAGE_MAX_BYTES   = 8192,
    RING_BUFFER_BYTES   = 4 * 256 + MESSAGE_MAX_BYTES,
    DICT_BYTES          = 8192,
};

typedef struct TaskParameters_t
{
	SemaphoreHandle_t *pcSemaphores;
	uint8_t buffer[LZ4_COMPRESSBOUND(MESSAGE_MAX_BYTES)];


} TaskParameters;

/* Application task prototypes. */
void prvCompressTask( void *pvParameters );
void prvDecompressTask( void *pvParameters );

/* FreeRTOS function/hook prototypes. */
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );

static uint32_t x=5238, y=234795, z=125805, w=165469;

static uint32_t xorshift128(void) {
    uint32_t t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) ^ t ^ (t >> 8);
}

void init_led( void )
{
    /* Enable GPIO port N */
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    /* Enable GPIO pin N0 */
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);
}

int main_rtos(void) {
	/* Variable declarations */
	static TaskParameters    taskParams      = {NULL, NULL};
	static SemaphoreHandle_t pcSemaphores[2] = {NULL, NULL};

    /* Set up clock for 120MHz */
    MAP_SysCtlClockFreqSet(
        SYSCTL_XTAL_25MHZ
      | SYSCTL_OSC_MAIN
      | SYSCTL_USE_PLL
      | SYSCTL_CFG_VCO_480,
      120000000);

    init_led();

    pcSemaphores[0] = xSemaphoreCreateBinary();
    pcSemaphores[1] = xSemaphoreCreateBinary();
    taskParams.pcSemaphores = &pcSemaphores[0];

    static uint32_t task_result = NULL;

    if( task_result =
        xTaskCreate(
            prvCompressTask,
            (portCHAR *)"prvCompressTask",
            configMINIMAL_STACK_SIZE,
            (void*)&taskParams,
            (tskIDLE_PRIORITY+1),
            NULL
        ) != pdTRUE)
    {
        /* Task not created.  Stop here for debug. */
        while (1);
    }

    if( task_result =
        xTaskCreate(
            prvDecompressTask,
            (portCHAR *)"prvDecompressTask",
            configMINIMAL_STACK_SIZE,
            (void*)&taskParams,
            (tskIDLE_PRIORITY+1),
            NULL
            )
        != pdTRUE)
    {
        /* Task not created.  Stop here for debug. */
        while (1);
    }


    vTaskStartScheduler();

    for(;;);

    return 0;
}


void prvCompressTask( void *pvParameters )
{
    static uint8_t raw_buffer[MESSAGE_MAX_BYTES] = {0};

    static LZ4_stream_t lz4Stream_body = { 0 };
	LZ4_stream_t* lz4Stream = &lz4Stream_body;

	SemaphoreHandle_t *sem_array;
	uint8_t	      *cmp_buffer;

	sem_array = (SemaphoreHandle_t*)(((TaskParameters*)pvParameters)->pcSemaphores);
	cmp_buffer = (uint8_t*)(((TaskParameters*)pvParameters)->buffer);

	int i = 0;

	for (;;)
	{
    	const int randomLength = (xorshift128() % MESSAGE_MAX_BYTES) + 1;
//		int randomLength = 126;

    	for (i = 0; i < randomLength; i++){
    		raw_buffer[i] = i;
    	}

    	/* Compress raw bytes to compress buffer */
    	const int cmpBytes = LZ4_compress_continue(
			lz4Stream,
			raw_buffer,
			cmp_buffer+sizeof(int),
			randomLength);

    	/* Copy number of bytes to 'header' position */
        memcpy(cmp_buffer, &cmpBytes, sizeof(int));

        /* Signal data ready */
		xSemaphoreGive(sem_array[0]);

		/* Wait for data clear */
		xSemaphoreTake(sem_array[1], portMAX_DELAY);
	}
}

void prvDecompressTask( void *pvParameters )
{

    static LZ4_streamDecode_t lz4StreamDecode_body = { 0 };
    LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;

    static uint8_t raw_buffer[MESSAGE_MAX_BYTES] = {0};

	SemaphoreHandle_t *sem_array;
	uint8_t	      *cmp_buffer;

	uint32_t cmpBytes;

	sem_array = (SemaphoreHandle_t*)(((TaskParameters*)pvParameters)->pcSemaphores);
	cmp_buffer = (uint8_t*)(((TaskParameters*)pvParameters)->buffer);

	for (;;)
	{
		/* Wait for data ready */
		xSemaphoreTake(sem_array[0], portMAX_DELAY);

		/* Toggle LED */
		ROM_GPIOPinWrite(
			GPIO_PORTN_BASE,
			GPIO_PIN_0,
			~ROM_GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_0)
		);

    	/* Copy number of bytes from 'header' position */
		memcpy(&cmpBytes, cmp_buffer, sizeof(int));

    	/* Deompress compressed bytes to raw buffer */
		const int decBytes = LZ4_decompress_safe_continue(
			lz4StreamDecode,
			cmp_buffer+sizeof(int),
			raw_buffer,
			cmpBytes,
			MESSAGE_MAX_BYTES);

		/* Do something with buffer before returning it */
		cmp_buffer[0] = 0;

		/* Simulate processing the data */
		vTaskDelay(500 / portTICK_RATE_MS);

		/* Signal data clear */
		xSemaphoreGive(sem_array[1]);
	}
}

void vApplicationMallocFailedHook( void )
{
    __asm("    nop\n");

}

void vApplicationIdleHook( void )
{
    __asm("    nop\n");

}

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;
    __asm("    nop\n");

}

void vApplicationTickHook( void )
{

    __asm("    nop\n");

}

void vAssertCalled( unsigned long ulLine, const char * const pcFileName )
{
    __asm("    nop\n");

}
