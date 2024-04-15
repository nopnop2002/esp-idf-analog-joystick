/*
	Example of reading data from ADC1
	This example code is in the Public Domain (or CC0 licensed, at your option.)
	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/message_buffer.h"
#include "esp_log.h"
#include "soc/adc_channel.h"
#include "esp_adc/adc_oneshot.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "cJSON.h"

extern QueueHandle_t xQueueCmd;
extern MessageBufferHandle_t xMessageBufferToClient;

const static char *TAG = "STICK";

#define GPIO_INPUT CONFIG_SW_GPIO

esp_err_t NVS_check_key(nvs_handle_t my_handle, char * key);
esp_err_t NVS_read_key(nvs_handle_t my_handle, char * key, int32_t *value);
esp_err_t NVS_write_key(nvs_handle_t my_handle, char * key, int32_t value);

int map(int x, int in_min, int in_max, int out_min, int out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// convert from gpio to adc1 channel
adc_channel_t gpio2adc(int gpio) {
#if CONFIG_IDF_TARGET_ESP32
	if (gpio == 32) return ADC1_GPIO32_CHANNEL;
	if (gpio == 33) return ADC1_GPIO33_CHANNEL;
	if (gpio == 34) return ADC1_GPIO34_CHANNEL;
	if (gpio == 35) return ADC1_GPIO35_CHANNEL;
	if (gpio == 36) return ADC1_GPIO36_CHANNEL;
	if (gpio == 37) return ADC1_GPIO37_CHANNEL;
	if (gpio == 38) return ADC1_GPIO38_CHANNEL;
	if (gpio == 39) return ADC1_GPIO39_CHANNEL;

#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
	if (gpio == 1) return ADC1_GPIO1_CHANNEL;
	if (gpio == 2) return ADC1_GPIO2_CHANNEL;
	if (gpio == 3) return ADC1_GPIO3_CHANNEL;
	if (gpio == 4) return ADC1_GPIO4_CHANNEL;
	if (gpio == 5) return ADC1_GPIO5_CHANNEL;
	if (gpio == 6) return ADC1_GPIO6_CHANNEL;
	if (gpio == 7) return ADC1_GPIO7_CHANNEL;
	if (gpio == 8) return ADC1_GPIO8_CHANNEL;
	if (gpio == 9) return ADC1_GPIO9_CHANNEL;
	if (gpio == 10) return ADC1_GPIO10_CHANNEL;

#else
	if (gpio == 0) return ADC1_GPIO0_CHANNEL;
	if (gpio == 1) return ADC1_GPIO1_CHANNEL;
	if (gpio == 2) return ADC1_GPIO2_CHANNEL;
	if (gpio == 3) return ADC1_GPIO3_CHANNEL;
	if (gpio == 4) return ADC1_GPIO4_CHANNEL;
	if (gpio == 5) return ADC1_GPIO5_CHANNEL;
	if (gpio == 6) return ADC1_GPIO6_CHANNEL;

#endif
	return -1;
}

void calibration(adc_oneshot_unit_handle_t adc1_handle, adc_channel_t adc1_channel_vrx, adc_channel_t adc1_channel_vry, int32_t *adc_avr) {
	int adc_raw[2];
	int32_t adc_sum[2];
	//int32_t adc_avr[2];
	adc_sum[0] = 0;
	adc_sum[1] = 0;
	for (int i=0;i<100;i++) {
		ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, adc1_channel_vrx, &adc_raw[0]));
		ESP_LOGD(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, adc1_channel_vrx, adc_raw[0]);
		adc_sum[0] = adc_sum[0] + adc_raw[0];

		ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, adc1_channel_vry, &adc_raw[1]));
		ESP_LOGD(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, adc1_channel_vry, adc_raw[1]);
		adc_sum[1] = adc_sum[1] + adc_raw[1];
		vTaskDelay(2);
	}
	adc_avr[0] = adc_sum[0] / 100;
	adc_avr[1] = adc_sum[1] / 100;
	ESP_LOGD(TAG, "adc_avr[0] = %"PRIi32" adc_avr[1]=%"PRIi32, adc_avr[0], adc_avr[1]);
}

void joy_stick(void *pvParameters)
{
	// ADC1 Init
	adc_oneshot_unit_handle_t adc1_handle;
	adc_oneshot_unit_init_cfg_t init_config1 = {
		.unit_id = ADC_UNIT_1,
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

	// ADC1 Config
	adc_oneshot_chan_cfg_t config = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,
		//.atten = ADC_ATTEN_DB_11,
		.atten = ADC_ATTEN_DB_12,
	};
	adc_channel_t adc1_channel_vrx = gpio2adc(CONFIG_VRX_GPIO);
	ESP_LOGI(TAG, "CONFIG_VRX_GPIO=%d adc1_channel_vrx=%d", CONFIG_VRX_GPIO, adc1_channel_vrx);
	adc_channel_t adc1_channel_vry = gpio2adc(CONFIG_VRY_GPIO);
	ESP_LOGI(TAG, "CONFIG_VRY_GPIO=%d adc1_channel_vry=%d", CONFIG_VRY_GPIO, adc1_channel_vry);
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, adc1_channel_vrx, &config));
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, adc1_channel_vry, &config));

	// Initialize GPIO
	gpio_reset_pin(GPIO_INPUT);
	gpio_set_direction(GPIO_INPUT, GPIO_MODE_INPUT);

	// Open NVS
	ESP_LOGI(TAG, "Opening Non-Volatile Storage handle... ");
	esp_err_t err;
	nvs_handle_t nvs_handle;
	err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Error (%s) opening NVS handle", esp_err_to_name(err));
		while(1) {
			vTaskDelay(1);
		}
	}
	ESP_LOGI(TAG, "Done");

	int32_t vrx_center = 0;
	int32_t vry_center = 0;
	int32_t vrx_low = 0;
	int32_t vry_low = 0;
	int32_t vrx_high = 0;
	int32_t vry_high = 0;
	int16_t cmd;

	// Check NVS
	esp_err_t ret_vrx_center = NVS_check_key(nvs_handle, "vrx_center");
	esp_err_t ret_vry_center = NVS_check_key(nvs_handle, "vry_center");
	esp_err_t ret_vrx_low = NVS_check_key(nvs_handle, "vrx_low");
	esp_err_t ret_vry_low = NVS_check_key(nvs_handle, "vry_low");
	esp_err_t ret_vrx_high = NVS_check_key(nvs_handle, "vrx_high");
	esp_err_t ret_vry_high = NVS_check_key(nvs_handle, "vry_high");
	ESP_LOGD(TAG, "ret_vrx_center=%d ret_vrx_low=%d ret_vrx_high=%d", ret_vrx_center, ret_vrx_low, ret_vrx_high);
	ESP_LOGD(TAG, "ret_vry_center=%d ret_vry_low=%d ret_vry_high=%d", ret_vry_center, ret_vry_low, ret_vry_high);
	if (ret_vrx_center == ESP_OK && ret_vrx_low == ESP_OK && ret_vrx_high == ESP_OK &&
		ret_vry_center == ESP_OK && ret_vry_low == ESP_OK && ret_vry_high == ESP_OK) {
		// Read NVS
		ESP_ERROR_CHECK(NVS_read_key(nvs_handle, "vrx_center", &vrx_center));
		ESP_ERROR_CHECK(NVS_read_key(nvs_handle, "vry_center", &vry_center));
		ESP_ERROR_CHECK(NVS_read_key(nvs_handle, "vrx_low", &vrx_low));
		ESP_ERROR_CHECK(NVS_read_key(nvs_handle, "vry_low", &vry_low));
		ESP_ERROR_CHECK(NVS_read_key(nvs_handle, "vrx_high", &vrx_high));
		ESP_ERROR_CHECK(NVS_read_key(nvs_handle, "vry_high", &vry_high));
		ESP_LOGI(TAG, "vrx_center=%"PRIi32" vrx_low=%"PRIi32" vrx_low=%"PRIi32, vrx_center, vrx_low, vrx_high);
		ESP_LOGI(TAG, "vry_center=%"PRIi32" vry_low=%"PRIi32" vry_low=%"PRIi32, vry_center, vry_low, vry_high);
	} else {
		// Do calibration
		int32_t adc_avr[2];
		ESP_LOGW(TAG, "Don't touch the joystick. Press Enter when you are ready.");
		xQueueReceive(xQueueCmd, &cmd, portMAX_DELAY);
		calibration(adc1_handle, adc1_channel_vrx, adc1_channel_vry, adc_avr);
		vrx_center = adc_avr[0];
		vry_center = adc_avr[1];

		ESP_LOGW(TAG, "Tilt the joystick all the way to the top. Press Enter when you are ready.");
		xQueueReceive(xQueueCmd, &cmd, portMAX_DELAY);
		calibration(adc1_handle, adc1_channel_vrx, adc1_channel_vry, adc_avr);
		vry_low = adc_avr[1];

		ESP_LOGW(TAG, "Tilt the joystick all the way to the buttom. Press Enter when you are ready.");
		xQueueReceive(xQueueCmd, &cmd, portMAX_DELAY);
		calibration(adc1_handle, adc1_channel_vrx, adc1_channel_vry, adc_avr);
		vry_high = adc_avr[1];

		ESP_LOGW(TAG, "Tilt the joystick all the way to the left. Press Enter when you are ready.");
		xQueueReceive(xQueueCmd, &cmd, portMAX_DELAY);
		calibration(adc1_handle, adc1_channel_vrx, adc1_channel_vry, adc_avr);
		vrx_low = adc_avr[0];

		ESP_LOGW(TAG, "Tilt the joystick all the way to the right. Press Enter when you are ready.");
		xQueueReceive(xQueueCmd, &cmd, portMAX_DELAY);
		calibration(adc1_handle, adc1_channel_vrx, adc1_channel_vry, adc_avr);
		vrx_high = adc_avr[0];

		ESP_ERROR_CHECK(NVS_write_key(nvs_handle, "vrx_center", vrx_center));
		ESP_ERROR_CHECK(NVS_write_key(nvs_handle, "vry_center", vry_center));
		ESP_ERROR_CHECK(NVS_write_key(nvs_handle, "vrx_low", vrx_low));
		ESP_ERROR_CHECK(NVS_write_key(nvs_handle, "vry_low", vry_low));
		ESP_ERROR_CHECK(NVS_write_key(nvs_handle, "vrx_high", vrx_high));
		ESP_ERROR_CHECK(NVS_write_key(nvs_handle, "vry_high", vry_high));

		ESP_LOGI(TAG, "vrx_center=%"PRIi32" vrx_low=%"PRIi32" vrx_low=%"PRIi32, vrx_center, vrx_low, vrx_high);
		ESP_LOGI(TAG, "vry_center=%"PRIi32" vry_low=%"PRIi32" vry_low=%"PRIi32, vry_center, vry_low, vry_high);
		ESP_LOGW(TAG, "Calibration Done. Press Enter when you are ready.");
		xQueueReceive(xQueueCmd, &cmd, portMAX_DELAY);
	}

	int vrx = 0;
	int vry = 0;
	int heading = 0;
	int sw_old = gpio_get_level(GPIO_INPUT);

    int adc_raw[2];
	while (1) {
		ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, adc1_channel_vrx, &adc_raw[0]));
		ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, adc1_channel_vrx, adc_raw[0]);

		ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, adc1_channel_vry, &adc_raw[1]));
		ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, adc1_channel_vry, adc_raw[1]);

		int sw_new = gpio_get_level(GPIO_INPUT);
		ESP_LOGD(TAG, "sw_new:%d sw_old=%d", sw_new, sw_old);
		if (sw_old == 1 && sw_new == 0) {
			heading = heading + 15;
			if (heading >= 360) heading = heading - 360;
		}
		sw_old = sw_new;

		if (adc_raw[0] <= vrx_center) {
			vrx = map(adc_raw[0], vrx_low, vrx_center, -90, 0);
		} else {
			vrx = map(adc_raw[0], vrx_center+1, vrx_high, 0, 90);
		}
		if (adc_raw[1] <= vry_center) {
			vry = map(adc_raw[1], vry_low, vry_center, -90, 0);
		} else {
			vry = map(adc_raw[1], vry_center+1, vry_high, 0, 90);
		}
		ESP_LOGI(TAG, "vrx:%d vry:%d heading=%d", vrx, vry, heading);

		// Send WEB request
		cJSON *request;
		request = cJSON_CreateObject();
		cJSON_AddStringToObject(request, "id", "data-request");
		cJSON_AddNumberToObject(request, "roll", vrx);
		cJSON_AddNumberToObject(request, "pitch", vry);
		cJSON_AddNumberToObject(request, "yaw", heading);
		char *my_json_string = cJSON_Print(request);
		ESP_LOGD(TAG, "my_json_string\n%s",my_json_string);
		size_t xBytesSent = xMessageBufferSend(xMessageBufferToClient, my_json_string, strlen(my_json_string), 100);
		if (xBytesSent != strlen(my_json_string)) {
			ESP_LOGE(TAG, "xMessageBufferSend fail");
		}
		cJSON_Delete(request);
		cJSON_free(my_json_string);

		vTaskDelay(pdMS_TO_TICKS(100));
	}

	// Never reach here
	ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
	vTaskDelete(NULL);
}
