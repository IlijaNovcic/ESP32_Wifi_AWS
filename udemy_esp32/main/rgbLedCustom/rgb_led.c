/*
 * rgb_led.c
 *
 *  Created on: 17.05.2023.
 *      Author: Novcic
 */

#include <stdbool.h>
#include "driver/ledc.h"
#include "rgb_led.h"

ledc_info_t ledc_ch[3u];
static bool	rgb_led_pwm_initialized = false;
/*
 * Initializes the RGB LED settings per channel, including
 * the GPIO for each color, mode and timer configuration.
 */

static void rgb_led_pwm_init(void)
{
	int rgb_ch;

	//Red
	ledc_ch[0u].channel 		 = LEDC_CHANNEL_0;
	ledc_ch[0u].gpio				 = RGB_LED_RED_GPIO;
	ledc_ch[0u].mode 			 = LEDC_HIGH_SPEED_MODE;
	ledc_ch[0u].timer_index = LEDC_TIMER_0;

	//Green
	ledc_ch[1u].channel 		 = LEDC_CHANNEL_1;
	ledc_ch[1u].gpio				 = RGB_LED_GREEN_GPIO;
	ledc_ch[1u].mode 			 = LEDC_HIGH_SPEED_MODE;
	ledc_ch[1u].timer_index = LEDC_TIMER_0;

	//Blue
	ledc_ch[2u].channel 		 = LEDC_CHANNEL_2;
	ledc_ch[2u].gpio				 = RGB_LED_BLUE_GPIO;
	ledc_ch[2u].mode 			 = LEDC_HIGH_SPEED_MODE;
	ledc_ch[2u].timer_index = LEDC_TIMER_0;

	//Configure timer 0
	ledc_timer_config_t ledc_timer =
	{
			.duty_resolution 			= LEDC_TIMER_8_BIT,
			.freq_hz							= 100u,
			.speed_mode 					= LEDC_HIGH_SPEED_MODE,
			.timer_num						=LEDC_TIMER_0
	};

	ledc_timer_config(&ledc_timer);

	for(rgb_ch = 0u; rgb_ch < RGB_LED_CH_NUM; rgb_ch++)
	{
		ledc_channel_config_t ledc_channel =
		{
				.channel 					= ledc_ch[rgb_ch].channel,
				.duty							= 0u,
				.hpoint 					= 0u,
				.gpio_num					= ledc_ch[rgb_ch].gpio,
				.intr_type				= LEDC_INTR_DISABLE,
				.speed_mode 	 		= ledc_ch[rgb_ch].mode,
				.timer_sel				= ledc_ch[rgb_ch].timer_index
		};

		ledc_channel_config(&ledc_channel);
	}
	rgb_led_pwm_initialized = true;
}

/*
 * Sets RGB color.
 */
static void rgb_led_set_color (uint8_t red, uint8_t green, uint8_t blue)
{
	//value should be 0-255 for 8 bit number
	ledc_set_duty(ledc_ch[0u].mode, ledc_ch[0u].channel, red);
	ledc_update_duty(ledc_ch[0u].mode, ledc_ch[0u].channel);

	//value should be 0-255 for 8 bit number
	ledc_set_duty(ledc_ch[1u].mode, ledc_ch[1u].channel, green);
	ledc_update_duty(ledc_ch[1u].mode, ledc_ch[1u].channel);

	//value should be 0-255 for 8 bit number
	ledc_set_duty(ledc_ch[2u].mode, ledc_ch[2u].channel, blue);
	ledc_update_duty(ledc_ch[2u].mode, ledc_ch[2u].channel);
}

void rgb_led_wifi_app_started(void)
{
	if(rgb_led_pwm_initialized == false)
	{
		rgb_led_pwm_init();
	}

	rgb_led_set_color(255u, 102u, 255u);
}

void rgb_led_http_server_started(void)
{
	if(rgb_led_pwm_initialized == false)
	{
		rgb_led_pwm_init();
	}

	rgb_led_set_color(204u, 255u, 51u);
}

void rgb_led_wifi_connected(void)
{
	if(rgb_led_pwm_initialized == false)
	{
		rgb_led_pwm_init();
	}

	rgb_led_set_color(0u, 255u, 153u);
}
