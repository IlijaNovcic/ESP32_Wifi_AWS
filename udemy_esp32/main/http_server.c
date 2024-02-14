/*
 * http_server.c
 *
 *  Created on: 19.05.2023.
 *      Author: Novcic
 */
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "sys/param.h"

#include "DHT22.h"
#include <sntp_time_sync.h>
#include "http_server.h"
#include "tasks_common.h"
#include "wifi_app.h"


//Tag used for ESP serial console messages
static const char TAG[] = "http_server";

//WIfi connect status
static uint32_t g_wifi_connect_status = NONE;

//Firmware update status
static int8_t g_fw_update_status = OTA_UPDATE_PENDING;

//Local time status
static bool g_is_local_time_set = false;

//HTTP server task handle
static httpd_handle_t http_server_handle = NULL;

//HTTP server monitor task handle
static TaskHandle_t task_http_server_monitor = NULL;

//Queue handle used to manipulate the main queue of events
static QueueHandle_t http_server_monitor_queue_handle;

/*
 * ESP32 timer configuration passed to esp_timer_create.
 */
const esp_timer_create_args_t fw_update_reset_args =
{
		.callback 				= &http_server_fw_update_reset_callback,
		.arg 							= NULL,
		.dispatch_method 	= ESP_TIMER_TASK,
		.name 						="fw_update_reset"
};

esp_timer_handle_t fw_update_reset;

// Embedded files : Everything in webpage package
extern const uint8_t jquery_3_3_1_min_js_start[] 	  				asm("_binary_jquery_3_3_1_min_js_start");
extern const uint8_t jquery_3_3_1_min_js_end[] 	  					asm("_binary_jquery_3_3_1_min_js_end");
extern const uint8_t index_html_start[]											asm("_binary_index_html_start");
extern const uint8_t index_html_end[]												asm("_binary_index_html_end");
extern const uint8_t app_css_start[]											  asm("_binary_app_css_start");
extern const uint8_t app_css_end[]										  		asm("_binary_app_css_end");
extern const uint8_t app_js_start[]													asm("_binary_app_js_start");
extern const uint8_t app_js_end[]														asm("_binary_app_js_end");
extern const uint8_t favicon_ico_start[]										asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[]											asm("_binary_favicon_ico_end");

/*
 * Checks the g_fw_update_status and creates the fw_update_reset_timer if g_fw_update_status is true
 */
static void http_server_fw_update_reset_timer(void)
{
	if(g_fw_update_status == OTA_UPDATE_SUCCESSFUL)
	{
		ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FW updated successful starting FW update reset timer");

		//Give the web page a chance to receive an acknowledge back and initialize the timer
		ESP_ERROR_CHECK(esp_timer_create(&fw_update_reset_args, &fw_update_reset));
		ESP_ERROR_CHECK(esp_timer_start_once(fw_update_reset, 8000000));
	}
	else
	{
		ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FW update unsuccessful");
	}
}

/*
 * HTTP server monitor task, used to track events of HTTP server
 * @param pvParameter parameter which can be passed to the task
 */
static void http_server_monitor(void *parameter)
{
	http_server_queue_message_t msg;

	for(;;)
	{
		if(xQueueReceive(http_server_monitor_queue_handle, &msg, portMAX_DELAY))
		{
			switch(msg.msgID)
			{
				case HTTP_MSG_WIFI_CONNECT_INIT:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_INIT");

					g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECTING;
					break;
				case HTTP_MSG_WIFI_CONNECT_SUCCES:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_SUCCES");

					g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_SUCCESS;

					break;
				case HTTP_MSG_WIFI_CONNECT_FAIL:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_FAIL");

					g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_FAILED;

					break;

				case HTTP_MSG_WIFI_USER_DISCONNECT:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_USER_DISCONNECT");

					g_wifi_connect_status = HTTP_WIFI_STATUS_DISCONNECTED;

					break;

				case HTTP_MSG_WIFI_CONNECT_OTA_UPDATE_SUCCESSUL:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_OTA_UPDATE_SUCCESSUL");
					g_fw_update_status = OTA_UPDATE_SUCCESSFUL;
					http_server_fw_update_reset_timer();

					break;
				case HTTP_MSG_WIFI_CONNECT_OTA_UPDATE_FAIL:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_OTA_UPDATE_FAIL");
					g_fw_update_status = OTA_UPDATE_FAILED;

					break;

				case HTTP_MSG_TIME_SERVICE_INITIALIZED:
					ESP_LOGI(TAG, "HTTP_MSG_TIME_SERVICE_INITIALIZED");
					g_is_local_time_set = true;

					break;

				default:
					break;
			}
		}
	}
}

/*
 * Jquery get handler is requested when accessing the web page.
 * @param req HTTP request for which uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_jqery_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "Jquery requested");

	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char*)jquery_3_3_1_min_js_start, jquery_3_3_1_min_js_end - jquery_3_3_1_min_js_start);

	return ESP_OK;
}

/*
 * Sends the index.html page.
 * @param req HTTP request for which uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_index_html_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "Index.html requested");

	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, (const char*)index_html_start, index_html_end - index_html_start);

	return ESP_OK;
}

/*
 * app.css get handler is requested when accessing the web page..
 * @param req HTTP request for which uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_app_css_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "app.css requested");

	httpd_resp_set_type(req, "text/css");
	httpd_resp_send(req, (const char*)app_css_start, app_css_end - app_css_start);

	return ESP_OK;
}

/*
 * app.js get handler is requested when accessing the web page..
 * @param req HTTP request for which uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_app_js_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "app.js requested");

	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char *)app_js_start, app_js_end - app_js_start);

	return ESP_OK;
}

/*
 * Sends the .ico (icon) file, when accessing the web page.
 * @param req HTTP request for which uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_favicon_ico_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "favicon.ico requested");

	httpd_resp_set_type(req, "image/x-icon");
	httpd_resp_send(req, (const char*)favicon_ico_start, favicon_ico_end - favicon_ico_start);

	return ESP_OK;
}

/*
 * Receives the .bin file via the web page and handles the firmware update
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK, otherwise ESP_FAIL if timeout occurs and the update cannot be started
 */
esp_err_t http_server_OTA_update_hanlder(httpd_req_t *req)
{
	esp_ota_handle_t ota_handle;

	char ota_buff[1024u];
	int32_t content_length = req->content_len;
	int32_t content_received = 0u;
	int32_t recv_length;
	bool is_req_body_started = false;
	bool flash_succesful = false;

	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

	do
	 {
		//Read the data for the request
		if((recv_length = httpd_req_recv(req,ota_buff, MIN(content_length, sizeof(ota_buff)))) < 0u)
		{
			//Check if timeout occured
			if(recv_length == HTTPD_SOCK_ERR_TIMEOUT)
			{
				ESP_LOGI(TAG, "http_server_OTA_update_handler: Socket Timeout");
				continue; ///> Retry receiving if timeout occurred
			}
			ESP_LOGI(TAG, "http_server_OTA_update_handler: OTA other error %d", recv_length);
			return ESP_FAIL;
		}
		printf("http_server_OTA_update_handler: OTA RX: %d of %d\r", content_received, content_length);

		//Is this the first data we are receiving
		//If so, it will have the information in the header that we need.
		if(!is_req_body_started)
		{
			is_req_body_started = true;

			//Get the location of the .bin file content (remove the web form data)
			char* body_start_p = strstr(ota_buff, "\r\n\r\n") + 4u;
			int32_t body_part_len = recv_length - (body_start_p - ota_buff);
			printf("http_server_OTA_update_handler: OTA file size: %d\r\n", content_length);

			esp_err_t err = esp_ota_begin(update_partition, 0xFFFFFFFF, &ota_handle);
			if(err != ESP_OK)
			{
				printf("http_server_OTA_update_handler: Error with OTA begin, cancelling OTA\r\n");
				return ESP_FAIL;
			}
			else
			{
				printf("http_server_OTA_update_handler: Writing to partition subtype %d at offset 0x%x\r\n", update_partition->subtype, update_partition->address);
			}

			//Write this first part of the data
			esp_ota_write(ota_handle, body_start_p, body_part_len);
			//Increase the content for the part that we wrote in first cycle
			content_received += body_part_len;
		}
		else
		{
			//Write OTA data
			esp_ota_write(ota_handle, ota_buff, recv_length);
			content_received += recv_length;
		}

	}while(recv_length > 0 && content_received < content_length);

	if(esp_ota_end(ota_handle) == ESP_OK)
	{
		//Lets update the partition
		if(esp_ota_set_boot_partition(update_partition) == ESP_OK)
		{
			const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
			ESP_LOGI(TAG, "http_server_OTA_update_handler: Next boot partition subtype %d at the offset 0x%x", boot_partition->subtype, boot_partition->address);
			flash_succesful = true;
		}
		else
		{
			ESP_LOGI(TAG, "http_server_OTA_update_handler: Flash ERROR!");
		}
	}
	else
	{
		ESP_LOGI(TAG, "http_server_OTA_update_handler: esp_ota_end ERROR!");
	}

	//We wont update global variables throughout the file, so send the message about it
	if(flash_succesful)
	{
		http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_OTA_UPDATE_SUCCESSUL);
	}
	else
	{
		http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_OTA_UPDATE_FAIL);
	}

	return ESP_OK;
}

/*
 * OTA status handler responds with the firmware update status after OTA update is started
 * and responds with the compile time/date when the page is first requested
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
esp_err_t http_server_OTA_status_hanlder (httpd_req_t *req)
{
	char otaJSON[100];

	ESP_LOGI(TAG, "OTAstatus requested");
	sprintf(otaJSON, "{\"ota_update_status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}", g_fw_update_status, __TIME__, __DATE__);

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, otaJSON, strlen(otaJSON));

	return ESP_OK;
}

/*
 * DHT sensor reading json handler responds with DHT22 sensor data
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_get_dht_sensor_reading_json_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "/dhtSensor.json requested");
	char dhtSensorJSON[100];

	sprintf(dhtSensorJSON, "{\"temp\":\"%.1f\",\"humidity\":\"%.1f\"}", getTemperature(), getHumidity());

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, dhtSensorJSON, strlen(dhtSensorJSON));

	return ESP_OK;
}

/*
 * wifiConnect.json handler is invoker after the conect button is pressed
 * and receiving the SSID and PASS entered by the user
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_wifi_connect_json_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "/wifiConnect.json requested");

	size_t len_ssid = 0;
	size_t len_pass = 0;
	char* ssid_str = NULL;
	char* pass_str = NULL;

	//Get SSID header
	len_ssid = httpd_req_get_hdr_value_len(req, "my-connect-ssid") + 1u;

	if(len_ssid > 1)
	{
		ssid_str = malloc(len_ssid);
		if(httpd_req_get_hdr_value_str(req, "my-connect-ssid", ssid_str, len_ssid) == ESP_OK)
		{
			ESP_LOGI(TAG, "http_server_wifi_connect_json_handler: Found header => my-connect-ssid: %s", ssid_str);
		}
	}

	//Get PASS header
	len_pass = httpd_req_get_hdr_value_len(req, "my-connect-pwd") +1;

	if(len_pass > 1)
	{
		pass_str = malloc(len_pass);
		if(httpd_req_get_hdr_value_str(req, "my-connect-pwd", pass_str, len_pass) == ESP_OK)
		{
			ESP_LOGI(TAG, "http_server_wifi_connect_json_handler: Found header => my-connect-pwd: %s", pass_str);
		}
	}

	//Update the wifi networks configuration and let the wifi application know

	wifi_config_t* wifi_config = wifi_app_get_wifi_config();
	memset(wifi_config, 0x00, sizeof(wifi_config_t));
	memcpy(wifi_config->sta.ssid, ssid_str, len_ssid);
	memcpy(wifi_config->sta.password, pass_str, len_pass);

	wifi_app_send_message(WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER);

	free(ssid_str);
	free(pass_str);

	return ESP_OK;
}

/*
 * Wifi connect status handler updates the connection status for the web page.
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_wifi_connect_status_json_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "/wifiConnectStatus requested");

	char statusJSON[100];

	sprintf(statusJSON, "{\"wifi_connect_status\":%d}", g_wifi_connect_status);

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, statusJSON, strlen(statusJSON));

	return ESP_OK;
}

/*
 * Wifi connect info handler updates the connection info for the web page.
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_get_wifi_connect_info_status_json_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "/wifiConnectInfo.json requested");

	char ipInfoJSON[200];
	memset(ipInfoJSON, 0x0u, sizeof(ipInfoJSON));

	char ip[IP4ADDR_STRLEN_MAX];
	char netmask[IP4ADDR_STRLEN_MAX];
	char gw[IP4ADDR_STRLEN_MAX];

	if(g_wifi_connect_status == HTTP_WIFI_STATUS_CONNECT_SUCCESS)
	{
		wifi_ap_record_t wifi_data;
		ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&wifi_data));
		char* ssid = (char*)wifi_data.ssid;

		esp_netif_ip_info_t ip_info;
		ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif_sta, &ip_info));
		esp_ip4addr_ntoa(&ip_info.ip		 , ip, IP4ADDR_STRLEN_MAX);
		esp_ip4addr_ntoa(&ip_info.netmask, netmask, IP4ADDR_STRLEN_MAX);
		esp_ip4addr_ntoa(&ip_info.gw		 , gw, IP4ADDR_STRLEN_MAX);

		sprintf(ipInfoJSON, "{\"ip\":\"%s\",\"netmask\":\"%s\",\"gw\":\"%s\",\"ap\":\"%s\"}", ip, netmask, gw, ssid);

	}

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, ipInfoJSON, strlen(ipInfoJSON));

	return ESP_OK;
}

/*
 * wifiDisconnect.json handler responds by sending a message to wifi application to disconnect.
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_wifi_disconnect_json_handler(httpd_req_t* req)
{
	ESP_LOGI(TAG, "/wifiDisconnect.json requested");

	wifi_app_send_message(WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT);
	return ESP_OK;
}

/*
 * localTime.json handler responds by sending the local time.
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_get_local_time_json_handler(httpd_req_t* req)
{
	ESP_LOGI(TAG, "/localTime.json requested");

	char localTimeJSON[100] = {0};

	if (g_is_local_time_set)
	{
		sprintf(localTimeJSON, "{\"time\":\"%s\"}", sntp_time_sync_get_time());
	}

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, localTimeJSON, strlen(localTimeJSON));

	return ESP_OK;
}

/*
 * apSSID.json handler responds by sending the ap SSID.
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_get_ap_ssid_json_handler(httpd_req_t* req)
{
	ESP_LOGI(TAG, "/apSSID.json requested");

	char  ssidJSON[100] = {0};

	wifi_config_t *wifi_config = wifi_app_get_wifi_config();
	esp_wifi_get_config(ESP_IF_WIFI_AP, wifi_config);
	char* ssid = (char*)wifi_config->ap.ssid;

	sprintf(ssidJSON, "{\"ssid\":\"%s\"}", ssid);
	httpd_resp_send(req, ssidJSON, strlen(ssidJSON));

	return ESP_OK;
}

 /* Sets up the default httpd server configuration.
 * @returns http server instance handle if successful, NULL otherwise.
 */
static httpd_handle_t http_server_configure(void)
{
	//Generate default configuration
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	//Create HTTP server monitor task
	xTaskCreatePinnedToCore(&http_server_monitor, "http_server_monitor", HTTP_SERVER_MONITOR_STACK_SIZE, NULL, HTTP_SERVER_MONITOR_PRIORITY, &task_http_server_monitor, HTTP_SERVER_MONITOR_CORE_ID);

	//Create the message queue
	http_server_monitor_queue_handle = xQueueCreate(3, sizeof(http_server_queue_message_t));

	//The core that the HTTP server will run on
	config.core_id = HTTP_SERVER_TASK_CORE_ID;

	//Adjust the default priority to 1 less that the wifi application
	config.task_priority = HTTP_SERVER_TASK_PRIORITY;

	//Bump up the stack size (default is 4096u)
	config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;

	//Increase uri handlers
	config.max_uri_handlers = 20u;

	//Increase the timeout limits
	config.recv_wait_timeout = 10u;
	config.send_wait_timeout = 10u;

	ESP_LOGI(TAG,
					 "http_server_configure: Starting server on port: '%d' with task priority: '%d'",
					 config.server_port,
					 config.task_priority);

	//Start the httpd server
	if(httpd_start(&http_server_handle, &config) == ESP_OK)
	{
		ESP_LOGI(TAG, "http_server_configure:  Registering URI handlers");

		//Register query handler
		httpd_uri_t jquery_js =
		{
			.uri 			= "/jquery-3.3.1.min.js",
			.method 	= HTTP_GET,
			.handler 	= http_server_jqery_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &jquery_js);

		//Register index.html
		httpd_uri_t index_html =
		{
			.uri 			= "/",
			.method 	= HTTP_GET,
			.handler 	= http_server_index_html_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &index_html);

		//Register app.css
		httpd_uri_t app_css =
		{
			.uri 			= "/app.css",
			.method 	= HTTP_GET,
			.handler 	= http_server_app_css_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &app_css);

		//Register app.js
		httpd_uri_t app_js =
		{
			.uri 			= "/app.js",
			.method 	= HTTP_GET,
			.handler 	= http_server_app_js_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &app_js);

		//Register favicon.ico
		httpd_uri_t favicon_ico =
		{
			.uri 			= "/favicon.ico",
			.method 	= HTTP_GET,
			.handler 	= http_server_favicon_ico_handler,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &favicon_ico);


		//Register OTAupdate handler
		httpd_uri_t OTA_update =
		{
			.uri = "/OTAupdate",
			.method = HTTP_POST,
			.handler = http_server_OTA_update_hanlder,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &OTA_update);

		//Register OTAupdate handler
		httpd_uri_t OTA_status =
		{
			.uri = "/OTAstatus",
			.method = HTTP_POST,
			.handler = http_server_OTA_status_hanlder,
			.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &OTA_status);

		//Register dhtSensor.json handler
		httpd_uri_t dht_sensor_json =
		{
			.uri = "/dhtSensor.json",
			.method = HTTP_GET,
			.handler = http_server_get_dht_sensor_reading_json_handler,
			.user_ctx = NULL
		};

		httpd_register_uri_handler(http_server_handle, &dht_sensor_json);

		//Register wifiConnect.json handler
		httpd_uri_t wifi_connect_json =
		{
			.uri = "/wifiConnect.json",
			.method = HTTP_POST,
			.handler = http_server_wifi_connect_json_handler,
			.user_ctx = NULL
		};

		httpd_register_uri_handler(http_server_handle, &wifi_connect_json);

		//Register wifiConnectStatus.json handler
		httpd_uri_t wifi_connect_status_json =
		{
			.uri = "/wifiConnectStatus",
			.method = HTTP_POST,
			.handler = http_server_wifi_connect_status_json_handler,
			.user_ctx = NULL
		};

		httpd_register_uri_handler(http_server_handle, &wifi_connect_status_json);

		//Register wifiConnectInfo.json handler
		httpd_uri_t wifi_connect_info_json =
		{
			.uri = "/wifiConnectInfo.json",
			.method = HTTP_GET,
			.handler = http_server_get_wifi_connect_info_status_json_handler,
			.user_ctx = NULL
		};

		httpd_register_uri_handler(http_server_handle, &wifi_connect_info_json);

		//Register wifiDisconnect.json handler
		httpd_uri_t wifi_disconnect_json =
		{
			.uri = "/wifiDisconnect.json",
			.method = HTTP_DELETE,
			.handler = http_server_wifi_disconnect_json_handler,
			.user_ctx = NULL
		};

		httpd_register_uri_handler(http_server_handle, &wifi_disconnect_json);

		//Register localTime.json handler
		httpd_uri_t local_time_json =
		{
			.uri = "/localTime.json",
			.method = HTTP_GET,
			.handler = http_server_get_local_time_json_handler,
			.user_ctx = NULL
		};

		httpd_register_uri_handler(http_server_handle, &local_time_json);

		//Register localTime.json handler
		httpd_uri_t ap_ssid_json =
		{
			.uri = "/apSSID.json",
			.method = HTTP_GET,
			.handler = http_server_get_ap_ssid_json_handler,
			.user_ctx = NULL
		};

		httpd_register_uri_handler(http_server_handle, &ap_ssid_json);

		return http_server_handle;
	}

	return NULL;
}

void http_server_start(void)
{
	if(http_server_handle == NULL)
	{
		http_server_handle = http_server_configure();
	}
}

void http_server_stop(void)
{
	if(http_server_handle)
	{
		httpd_stop(http_server_handle);
		ESP_LOGI(TAG, "http_server_stop: stopping HTTP server");
		http_server_handle = NULL;
	}

	if(task_http_server_monitor)
	{
		vTaskDelete(task_http_server_monitor);
		ESP_LOGI(TAG, "http_server_stop: stopping HTTP server monitor");
		task_http_server_monitor = NULL;
	}
}

BaseType_t http_server_monitor_send_message(http_server_message_e msgID)
{
	http_server_queue_message_t msg;
	msg.msgID = msgID;
	return xQueueSend(http_server_monitor_queue_handle, &msg, portMAX_DELAY);
}

void http_server_fw_update_reset_callback(void* arg)
{
	ESP_LOGI(TAG, "http_server_fw_update_reset_callback: Timer timed-out, restarting the device");
	esp_restart();
}
