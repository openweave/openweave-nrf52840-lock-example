/*
 *
 *    Copyright (c) 2019 Google LLC.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// ----- Memory Config -----

#define MEM_MANAGER_ENABLED 1
#define MEMORY_MANAGER_SMALL_BLOCK_COUNT 4
#define MEMORY_MANAGER_SMALL_BLOCK_SIZE 32
#define MEMORY_MANAGER_MEDIUM_BLOCK_COUNT 4
#define MEMORY_MANAGER_MEDIUM_BLOCK_SIZE 256
#define MEMORY_MANAGER_LARGE_BLOCK_COUNT 1
#define MEMORY_MANAGER_LARGE_BLOCK_SIZE 1024

// ----- Crypto Config -----

#define NRF_CRYPTO_ENABLED 0

// ----- Soft Device Config -----

#define SOFTDEVICE_PRESENT 1
#define NRF_SDH_ENABLED 1
#define NRF_SDH_SOC_ENABLED 1
#define NRF_SDH_BLE_ENABLED 1
#define NRF_SDH_BLE_PERIPHERAL_LINK_COUNT 1
#define NRF_SDH_BLE_VS_UUID_COUNT 2
#define NRF_BLE_GATT_ENABLED 1
#define NRF_SDH_BLE_GATT_MAX_MTU_SIZE 251
#define NRF_SDH_BLE_GAP_DATA_LENGTH 251

// ----- FDS / Flash Config -----

#define FDS_ENABLED 1
#define FDS_BACKEND NRF_FSTORAGE_SD
#define NRF_FSTORAGE_ENABLED 1
// Number of virtual flash pages used for FDS data storage.
// NOTE: This value must correspond to FDS_FLASH_PAGES specified in
// linker directives (.ld) file.
#define FDS_VIRTUAL_PAGES 2

// ----- Logging Config -----

#define NRF_LOG_ENABLED 1
#define NRF_LOG_DEFAULT_LEVEL 4
#define NRF_LOG_DEFERRED 0
#define NRF_LOG_STR_PUSH_BUFFER_SIZE 16
#define NRF_LOG_USES_TIMESTAMP 1
#define NRF_LOG_STR_FORMATTER_TIMESTAMP_FORMAT_ENABLED 0

#define NRF_LOG_BACKEND_RTT_ENABLED 1
#define NRF_LOG_BACKEND_UART_ENABLED 0

#if NRF_LOG_BACKEND_UART_ENABLED

#define NRF_LOG_BACKEND_UART_TX_PIN 6
#define NRF_LOG_BACKEND_UART_BAUDRATE 30801920
#define NRF_LOG_BACKEND_UART_TEMP_BUFFER_SIZE 64

#define UART_ENABLED 1
#define UART0_ENABLED 1
#define NRFX_UARTE_ENABLED 1
#define NRFX_UART_ENABLED 1
#define UART_LEGACY_SUPPORT 0

#endif // NRF_LOG_BACKEND_UART_ENABLED

#if NRF_LOG_BACKEND_RTT_ENABLED

#define SEGGER_RTT_CONFIG_BUFFER_SIZE_UP 4096
#define SEGGER_RTT_CONFIG_MAX_NUM_UP_BUFFERS 1
#define SEGGER_RTT_CONFIG_BUFFER_SIZE_DOWN 16
#define SEGGER_RTT_CONFIG_MAX_NUM_DOWN_BUFFERS 1

// <0=> SKIP
// <1=> TRIM
// <2=> BLOCK_IF_FIFO_FULL
#define SEGGER_RTT_CONFIG_DEFAULT_MODE 1

#define NRF_LOG_BACKEND_RTT_TEMP_BUFFER_SIZE 64
#define NRF_LOG_BACKEND_RTT_TX_RETRY_DELAY_MS 1
#define NRF_LOG_BACKEND_RTT_TX_RETRY_CNT 3

#endif // NRF_LOG_BACKEND_RTT_ENABLED

// ----- Misc Config -----

#define NRF_CLOCK_ENABLED 1
#define NRF_FPRINTF_ENABLED 1
#define NRF_STRERROR_ENABLED 1
#define NRF_QUEUE_ENABLED 1
#define APP_TIMER_ENABLED 1
#define BUTTON_ENABLED 1

#define GPIOTE_ENABLED 1
#define GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS 4

#define APP_TIMER_CONFIG_OP_QUEUE_SIZE 20

// ---- Lock Example App Config ----

#define SERVICELESS_DEVICE_DISCOVERY_ENABLED    0
#define WEAVE_DEVICE_CONFIG_DISABLE_ACCOUNT_PAIRING     SERVICELESS_DEVICE_DISCOVERY_ENABLED

// If account pairing is disabled, device will not be able to subscribe to service.
// Defaults provided for auto relock enable and auto lock duration
#if WEAVE_DEVICE_CONFIG_DISABLE_ACCOUNT_PAIRING
#define AUTO_RELOCK_ENABLED_DEFAULT             1
#define AUTO_LOCK_DURATION_SECS_DEFAULT         10
#define DOOR_CHECK_ENABLE_DEFAULT               1
#endif

#define ATTENTION_BUTTON                        BUTTON_3
#define LOCK_BUTTON                             BUTTON_2
#define FUNCTION_BUTTON                         BUTTON_1
#define FUNCTION_BUTTON_DEBOUNCE_PERIOD_MS      50
#define LONG_PRESS_TIMEOUT_MS                   (3000)

#define SYSTEM_STATE_LED                        BSP_LED_0
#define LOCK_STATE_LED                          BSP_LED_1

// Time it takes in ms for the simulated actuator to move from one
// state to another.
#define ACTUATOR_MOVEMENT_PERIOS_MS             2000

// ---- Lock Example SWU Config ----
#define SWU_INTERVAl_WINDOW_MIN_MS              (23*60*60*1000) // 23 hours
#define SWU_INTERVAl_WINDOW_MAX_MS              (24*60*60*1000) // 24 hours

// ---- Thread Polling Config ----
#define THREAD_ACTIVE_POLLING_INTERVAL_MS       100
#define THREAD_INACTIVE_POLLING_INTERVAL_MS     1000

// ---- Device to Service Subscription Config ----

/** Defines the timeout for liveness between the service and the device.
 *  For sleepy end node devices, this timeout will be much larger than the current
 *  value to preserve battery.
 */
#define SERVICE_LIVENESS_TIMEOUT_SEC            60 * 1 // 1 minute

/** Defines the timeout for a response to any message initiated by the device to the service.
 *  This includes notifies, subscribe confirms, cancels and updates.
 *  This timeout is kept SERVICE_WRM_MAX_RETRANS x SERVICE_WRM_INITIAL_RETRANS_TIMEOUT_MS + some buffer
 *  to account for latency in the message transmission through multiple hops.
 */
#define SERVICE_MESSAGE_RESPONSE_TIMEOUT_MS     10000

/** Defines the timeout for a message to get retransmitted when no wrm ack or
 *  response has been heard back from the service. This timeout is kept larger
 *  for now since the message has to travel through multiple hops and service
 *  layers before actually making it to the actual receiver.
 *  @note
 *    WRM has an initial and active retransmission timeouts to interact with
 *    sleepy destination nodes.
 */
#define SERVICE_WRM_INITIAL_RETRANS_TIMEOUT_MS  2500

#define SERVICE_WRM_ACTIVE_RETRANS_TIMEOUT_MS   2500

/** Define the maximum number of retransmissions in WRM
 */
#define SERVICE_WRM_MAX_RETRANS                 3

/** Define the timeout for piggybacking a WRM acknowledgment message
 */
#define SERVICE_WRM_PIGGYBACK_ACK_TIMEOUT_MS    200

/** Defines the timeout for expecting a subscribe response after sending a subscribe request.
 *  This is meant to be a gross timeout - the MESSAGE_RESPONSE_TIMEOUT_MS will usually trip first
 *  to catch timeouts for each message in the subscribe request exchange.
 *  SUBSCRIPTION_RESPONSE_TIMEOUT_MS > Average no. of notifies during a subscription * MESSAGE_RESPONSE_TIMEOUT_MS
 */
#define SUBSCRIPTION_RESPONSE_TIMEOUT_MS        40000

// ---- Device to Device Subscription Config ----

/** Defines the timeout for liveness between the initiating device and the publishing device.
 *  For sleepy end node devices, this timeout may be much larger than the current
 *  value to preserve battery.
 */
#define DEVICE_LIVENESS_TIMEOUT_SEC             10 // 10 seconds

/** Defines the timeout for a response to any message sent by initiating device to the publishing device.
 *  This includes notifies, subscribe confirms, cancels and updates.
 *  This timeout is kept SERVICE_WRM_MAX_RETRANS x SERVICE_WRM_INITIAL_RETRANS_TIMEOUT_MS + some buffer
 *  to account for latency in the message transmission through multiple hops.
 */
#define DEVICE_MESSAGE_RESPONSE_TIMEOUT_MS      3000

/** Defines the timeout for a message to get retransmitted when no wrm ack or
 *  response has been heard back from the publishing device.
 */
#define DEVICE_WRM_INITIAL_RETRANS_TIMEOUT_MS   800

#define DEVICE_WRM_ACTIVE_RETRANS_TIMEOUT_MS    500

/** Define the maximum number of retransmissions in WRM
 */
#define DEVICE_WRM_MAX_RETRANS                  3

/** Define the timeout for piggybacking a WRM acknowledgment message
 */
#define DEVICE_WRM_PIGGYBACK_ACK_TIMEOUT_MS     200

#endif //APP_CONFIG_H
