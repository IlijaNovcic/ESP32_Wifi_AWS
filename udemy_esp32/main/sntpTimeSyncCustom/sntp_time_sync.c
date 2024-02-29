/*
 * sntp_tyme_sync.c
 *
 *  Created on: 12.07.2023.
 *      Author: Novcic
 */
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/apps/sntp.h"

#include <sntp_time_sync.h>
#include "tasks_common.h"
#include "http_server.h"
#include "wifiCustom/wifi_app.h"

static const char TAG[] = "sntp_time_sync";

//SNTP operating mode status
static bool sntp_op_mode_set = false;

/*
 * Inizialize SNTP service using SNTP_OPMODE_POLL mode.
 */
static void sntp_time_sync_init_sntp(void)
{
	ESP_LOGI(TAG, "Initializing the SNTP service");

	if(!sntp_op_mode_set)
	{
		// Set the operating mode
		sntp_setoperatingmode(SNTP_OPMODE_POLL);
		sntp_op_mode_set = true;
	}

	sntp_setservername(0u, "pool.ntp.org");

	//initialize the service
	sntp_init();

	//Let http_server know service is initialized
	http_server_monitor_send_message(HTTP_MSG_TIME_SERVICE_INITIALIZED);
}

/*
 * Gets the current time and if the time is not up to date
 * the sntp_time_synch_init_sntp is called
 */
static void sntp_time_sync_obtain_time(void)
{
	time_t now = 0;
	struct tm time_info = {0};

	time(&now);
	localtime_r(&now, &time_info);

	//check the time, in case we need to initialize/reinitialize
	if(time_info.tm_year < (2016 - 1900))
	{
		sntp_time_sync_init_sntp();

		//Set the local time zone
		setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/3", 1); //TODO: Search for different Time Zone - In pdf file from resources
		tzset();
	}
}
/*
 * The SNTP time sync task
 * @param arg pvParam.
 */
static void sntp_tyme_sync(void* pvParam)
{
	while(1)
	{
		sntp_time_sync_obtain_time();
		vTaskDelay(10000 / portTICK_PERIOD_MS);
	}

	vTaskDelete(NULL);
}

char* sntp_time_sync_get_time(void)
{
	static char time_buffer[100] = {0};
	time_t now = 0;
	struct tm time_info = {0};

	time(&now);
	localtime_r(&now, &time_info);

	//check the time, in case we need to initialize/reinitialize
	if(time_info.tm_year < (2016 - 1900))
	{
		ESP_LOGI(TAG, "Time is not set yet");

	}
	else
	{
		strftime(time_buffer, sizeof(time_buffer), "%d.%m.%Y %H.%M.%S", &time_info);
		ESP_LOGI(TAG, "Current time info: %s", time_buffer);
	}

	return time_buffer;
}

void sntp_time_sync_task_start(void)
{
	xTaskCreatePinnedToCore(&sntp_tyme_sync, "sntp_tyme_sync", SNTP_TIME_SYNC_TASK_STACK_SIZE, NULL, SNTP_TIME_SYNC_TASK_PRIORITY, NULL, SNTP_TIME_SYNC_TASK_CORE_ID);
}
