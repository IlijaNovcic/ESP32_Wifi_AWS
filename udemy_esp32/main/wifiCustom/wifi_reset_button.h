/*
 * wifi_reset_button.h
 *
 *  Created on: 12.07.2023.
 *      Author: Novcic
 */

#ifndef MAIN_WIFI_RESET_BUTTON_H_
#define MAIN_WIFI_RESET_BUTTON_H_

//Default Interrupt flag
#define ESP_INTR_FLAG_DEFAULT   0u

//Wifi reset button is the BOOT button on DevKit
#define WIFI_RESET_BUTTON				0u

/*
 * Configure wifi reset button and interrupt configuration
 */
void wifi_reset_button_config(void);

#endif /* MAIN_WIFI_RESET_BUTTON_H_ */
