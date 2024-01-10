/* Non-Volatile Storage (NVS) Read and Write a Value Example

   For other examples please check:
   https://github.com/espressif/esp-idf/tree/master/examples

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

static char *TAG = "NVS";

esp_err_t NVS_check_key(nvs_handle_t my_handle, char * key) {
	ESP_LOGD(TAG, "NVS_KEY_NAME_MAX_SIZE=%d", NVS_KEY_NAME_MAX_SIZE);
	if (strlen(key) > NVS_KEY_NAME_MAX_SIZE-1) {
		ESP_LOGE(TAG, "Maximal length is %d", NVS_KEY_NAME_MAX_SIZE-1);
		return ESP_ERR_INVALID_ARG;
	}

	ESP_LOGD(TAG, "Checking %s from NVS ... ", key);
	int32_t value = 0; // value will default to 0, if not set yet in NVS
	esp_err_t err = nvs_get_i32(my_handle, key, &value);
	switch (err) {
		case ESP_OK:
			ESP_LOGD(TAG, "Done. %s = %"PRIi32, key, value);
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			ESP_LOGE(TAG, "The value is not initialized yet!");
			break;
		default :
			ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
			break;
	}
	return err;
}

esp_err_t NVS_read_key(nvs_handle_t my_handle, char * key, int32_t *value) {
	ESP_LOGD(TAG, "NVS_KEY_NAME_MAX_SIZE=%d", NVS_KEY_NAME_MAX_SIZE);
	if (strlen(key) > NVS_KEY_NAME_MAX_SIZE-1) {
		ESP_LOGE(TAG, "Maximal length is %d", NVS_KEY_NAME_MAX_SIZE-1);
		return ESP_ERR_INVALID_ARG;
	}

	ESP_LOGD(TAG, "Reading %s from NVS ... ", key);
	int32_t _value = 0; // value will default to 0, if not set yet in NVS
	esp_err_t err = nvs_get_i32(my_handle, key, &_value);
	switch (err) {
		case ESP_OK:
			ESP_LOGD(TAG, "Done. %s = %"PRIi32, key, _value);
			*value = _value;
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			ESP_LOGE(TAG, "The value is not initialized yet!");
			break;
		default :
			ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
			break;
	}
	return err;
}

esp_err_t NVS_write_key(nvs_handle_t my_handle, char * key, int32_t value) {
	ESP_LOGD(TAG, "NVS_KEY_NAME_MAX_SIZE=%d", NVS_KEY_NAME_MAX_SIZE);
	if (strlen(key) > NVS_KEY_NAME_MAX_SIZE-1) {
		ESP_LOGE(TAG, "Maximal length is %d", NVS_KEY_NAME_MAX_SIZE-1);
		return ESP_ERR_INVALID_ARG;
	}

	esp_err_t err = nvs_set_i32(my_handle, key, value);
	ESP_LOGD(TAG, "nvs_set_i32 err=%d", err);
	if (err == ESP_OK) {
		// Commit writte value.
		// After setting any values, nvs_commit() must be called to ensure changes are written
		// to flash storage. Implementations may write to storage at other times,
		// but this is not guaranteed.
		ESP_LOGD(TAG, "Committing updates in NVS ... ");
		err = nvs_commit(my_handle);
		if (err != ESP_OK) ESP_LOGE(TAG, "nvs_commit err=%d", err);
	}
	return err;
}

esp_err_t NVS_delete_key(nvs_handle_t my_handle, char * key) {
	ESP_LOGD(TAG, "NVS_KEY_NAME_MAX_SIZE=%d", NVS_KEY_NAME_MAX_SIZE);
	if (strlen(key) > NVS_KEY_NAME_MAX_SIZE-1) {
		ESP_LOGE(TAG, "Maximal length is %d", NVS_KEY_NAME_MAX_SIZE-1);
		return ESP_ERR_INVALID_ARG;
	}

	esp_err_t err = nvs_erase_key(my_handle, key);
	if (err != ESP_OK) ESP_LOGE(TAG, "nvs_erase_key err=%d", err);
	return err;
}
