/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/message_buffer.h"
#include "esp_log.h"
#include "soc/adc_channel.h"
#if ESP_IDF_VERSION_MAJOR == 5 && ESP_IDF_VERSION_MINOR == 0
#include "driver/adc.h" //  Need legacy adc driver for ADC1_GPIOxx_CHANNEL
#endif
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"
#include "cJSON.h"

extern MessageBufferHandle_t xMessageBufferToClient;

const static char *TAG = "STICK";

#if 0
/*---------------------------------------------------------------
		ADC General Macros
---------------------------------------------------------------*/
//ADC1 Channels
#if CONFIG_IDF_TARGET_ESP32
#define EXAMPLE_ADC1_CHAN0			ADC_CHANNEL_4
#define EXAMPLE_ADC1_CHAN1			ADC_CHANNEL_5
#else
#define EXAMPLE_ADC1_CHAN0			ADC_CHANNEL_2
#define EXAMPLE_ADC1_CHAN1			ADC_CHANNEL_3
#endif
#endif

#define GPIO_INPUT CONFIG_SW_GPIO

static int adc_raw[2][10];
static int voltage[2][10];
static bool example_adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void example_adc_calibration_deinit(adc_cali_handle_t handle);


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

#elif CONFIG_IDF_TARGET_ESP32C2 || CONFIG_IDF_TARGET_ESP32C3
	if (gpio == 0) return ADC1_GPIO0_CHANNEL;
	if (gpio == 1) return ADC1_GPIO1_CHANNEL;
	if (gpio == 2) return ADC1_GPIO2_CHANNEL;
	if (gpio == 3) return ADC1_GPIO3_CHANNEL;
	if (gpio == 4) return ADC1_GPIO4_CHANNEL;

#endif
	return -1;
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
		.atten = ADC_ATTEN_DB_11,
	};
	adc_channel_t adc1_channel_vrx = gpio2adc(CONFIG_VRX_GPIO);
	ESP_LOGI(TAG, "CONFIG_VRX_GPIO=%d adc1_channel_vrx=%d", CONFIG_VRX_GPIO, adc1_channel_vrx);
	adc_channel_t adc1_channel_vry = gpio2adc(CONFIG_VRY_GPIO);
	ESP_LOGI(TAG, "CONFIG_VRY_GPIO=%d adc1_channel_vry=%d", CONFIG_VRY_GPIO, adc1_channel_vry);
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, adc1_channel_vrx, &config));
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, adc1_channel_vry, &config));

	// ADC1 Calibration Init
	adc_cali_handle_t adc1_cali_handle = NULL;
	bool do_calibration = example_adc_calibration_init(ADC_UNIT_1, ADC_ATTEN_DB_11, &adc1_cali_handle);
	if (!do_calibration) {
		vTaskDelete(NULL);
	}

	// Initialize GPIO
	gpio_reset_pin(GPIO_INPUT);
	gpio_set_direction(GPIO_INPUT, GPIO_MODE_INPUT);

	int vrx = 0;
	int vry = 0;
	int heading = 0;
	int sw_old = gpio_get_level(GPIO_INPUT);

	while (1) {
		ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, adc1_channel_vrx, &adc_raw[0][0]));
		ESP_LOGD(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, adc1_channel_vrx, adc_raw[0][0]);
		ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, adc_raw[0][0], &voltage[0][0]));
		ESP_LOGD(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, adc1_channel_vrx, voltage[0][0]);
		//vTaskDelay(pdMS_TO_TICKS(1000));

		ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, adc1_channel_vry, &adc_raw[0][1]));
		ESP_LOGD(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, adc1_channel_vry, adc_raw[0][1]);
		ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, adc_raw[0][1], &voltage[0][1]));
		ESP_LOGD(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, adc1_channel_vry, voltage[0][1]);
		ESP_LOGD(TAG, "Voltage: %d %d", voltage[0][0], voltage[0][1]);

		int sw_new = gpio_get_level(GPIO_INPUT);
		ESP_LOGD(TAG, "sw_new:%d sw_old=%d", sw_new, sw_old);
		if (sw_old == 1 && sw_new == 0) {
			heading = heading + 15;
			if (heading >= 360) heading = heading - 360;
		}
		sw_old = sw_new;
		vrx = map(voltage[0][0], 0, 3300, -90, 90);
		vry = map(voltage[0][1], 0, 3300, -90, 90);
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
	example_adc_calibration_deinit(adc1_cali_handle);
	vTaskDelete(NULL);
}


/*---------------------------------------------------------------
		ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
	adc_cali_handle_t handle = NULL;
	esp_err_t ret = ESP_FAIL;
	bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
	if (!calibrated) {
		ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
		adc_cali_curve_fitting_config_t cali_config = {
			.unit_id = unit,
			.atten = atten,
			.bitwidth = ADC_BITWIDTH_DEFAULT,
		};
		ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
		if (ret == ESP_OK) {
			calibrated = true;
		}
	}
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
	if (!calibrated) {
		ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
		adc_cali_line_fitting_config_t cali_config = {
			.unit_id = unit,
			.atten = atten,
			.bitwidth = ADC_BITWIDTH_DEFAULT,
		};
		ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
		if (ret == ESP_OK) {
			calibrated = true;
		}
	}
#endif

	*out_handle = handle;
	if (ret == ESP_OK) {
		ESP_LOGI(TAG, "Calibration Success");
	} else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
		ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
	} else {
		ESP_LOGE(TAG, "Invalid arg or no memory");
	}

	return calibrated;
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
	ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
	ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
	ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
	ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}
