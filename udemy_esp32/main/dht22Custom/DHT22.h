/* 

	DHT22 temperature sensor driver

*/

#ifndef DHT22_H_  
#define DHT22_H_
#include <stdbool.h>

#define DHT_OK 							0u
#define DHT_CHECKSUM_ERROR -1u
#define DHT_TIMEOUT_ERROR  -2u

#define DHT_GPIO					 25u

/*
 * Starts DHT22 sensor task
 */
void DHT22_Task_Start(void);

// == function prototypes =======================================

void 	setDHTgpio(int gpio);
void 	errorHandler(int response);
int 	readDHT();
float getHumidity();
float getTemperature();
int 	getSignalLevel( int usTimeOut, bool state );

#endif
