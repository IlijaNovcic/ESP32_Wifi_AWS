/*
 * rgb_led.h
 *
 *  Created on: 17.05.2023.
 *      Author: Novcic
 */

#ifndef MAIN_RGB_LED_H_
#define MAIN_RGB_LED_H_

#include <stdint.h>

//RGB LED GPIOs
#define RGB_LED_RED_GPIO 		21u
#define RGB_LED_GREEN_GPIO	22u
#define RGB_LED_BLUE_GPIO		23u

//RGB LED color mix channels
#define RGB_LED_CH_NUM		  3u

//RGB LED configuration
typedef struct
{
		uint8_t channel;
		uint8_t gpio;
		uint8_t mode;
		uint8_t timer_index;
}ledc_info_t;

/*
 * Color to indicate Wifi application has started.
 */
void rgb_led_wifi_app_started(void);

/*
 * Color to indicate HTTP server has started.
 */
void rgb_led_http_server_started(void);

/*
 * Color to indicate Wifi application has started.
 */
void rgb_led_wifi_connected(void);

#endif /* MAIN_RGB_LED_H_ */
