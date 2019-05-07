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

/**
 *    @file
 *          Example project configuration file for OpenWeave.
 *
 *          This is a place to put application or project-specific overrides
 *          to the default configuration values for general OpenWeave features.
 *
 */

#ifndef WEAVE_PROJECT_CONFIG_H
#define WEAVE_PROJECT_CONFIG_H

// Use the default device id 18B4300000000003 if one hasn't been provisioned in flash.
#define WEAVE_DEVICE_CONFIG_ENABLE_TEST_DEVICE_IDENTITY 34

// Use a default pairing code if one hasn't been provisioned in flash.
#define WEAVE_DEVICE_CONFIG_USE_TEST_PAIRING_CODE "NESTUS"

// For convenience, enable Weave Security Test Mode and disable the requirement for
// authentication in various protocols.
//
//    WARNING: These options make it possible to circumvent basic Weave security functionality,
//    including message encryption. Because of this they MUST NEVER BE ENABLED IN PRODUCTION BUILDS.
//
#define WEAVE_CONFIG_SECURITY_TEST_MODE 1
#define WEAVE_CONFIG_REQUIRE_AUTH 0

/**
 * WEAVE_DEVICE_CONFIG_DEVICE_VENDOR_ID
 *
 * Use yale vendor id for now
 * TODO: Update to Google Vendor id when resource is available in the cloud
 */
#define WEAVE_DEVICE_CONFIG_DEVICE_VENDOR_ID 0xE100

/**
 * WEAVE_DEVICE_CONFIG_DEVICE_PRODUCT_ID
 *
 * The unique id assigned by the device vendor to identify the product or device type.  This
 * number is scoped to the device vendor id.
 */
#define WEAVE_DEVICE_CONFIG_DEVICE_PRODUCT_ID 0xFE00

/**
 * WEAVE_DEVICE_CONFIG_DEVICE_PRODUCT_REVISION
 *
 * The product revision number assigned to device or product by the device vendor.  This
 * number is scoped to the device product id, and typically corresponds to a revision of the
 * physical device, a change to its packaging, and/or a change to its marketing presentation.
 * This value is generally *not* incremented for device software revisions.
 */
#define WEAVE_DEVICE_CONFIG_DEVICE_PRODUCT_REVISION 1

/**
 * WEAVE_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION
 *
 * A string identifying the firmware revision running on the device.
 */
#define WEAVE_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION "1.0d1"

/**
 * WEAVE_DEVICE_CONFIG_ENABLE_WOBLE
 *
 * Enable support for Weave-over-BLE (WoBLE).
 */
#define WEAVE_DEVICE_CONFIG_ENABLE_WOBLE 1

/**
 * WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC
 *
 * Enables synchronizing the device's real time clock with a remote Weave Time service
 * using the Weave Time Sync protocol.
 */
#define WEAVE_DEVICE_CONFIG_ENABLE_WEAVE_TIME_SERVICE_TIME_SYNC 1

/**
 * WEAVE_DEVICE_CONFIG_SERVICE_PROVISIONING_CONNECTIVITY_TIMEOUT
 *
 * The maximum amount of time (in milliseconds) to wait for service connectivity during the device
 * service provisioning step.  More specifically, this is the maximum amount of time the device will
 * wait for connectivity to be established with the service at the point where the device waiting
 * to send a Pair Device to Account request to the Service Provisioning service.
 * TODO: Revert this back to default values
 */
#define WEAVE_DEVICE_CONFIG_SERVICE_PROVISIONING_CONNECTIVITY_TIMEOUT 300000

/**
 * WEAVE_DEVICE_CONFIG_SERVICE_PROVISIONING_REQUEST_TIMEOUT
 *
 * Specifies the maximum amount of time (in milliseconds) to wait for a response from the Service
 * Provisioning service.
 * TODO: Revert this back to default values
 */
#define WEAVE_DEVICE_CONFIG_SERVICE_PROVISIONING_REQUEST_TIMEOUT 300000

/**
 * WEAVE_DEVICE_CONFIG_DISABLE_ACCOUNT_PAIRING
 *
 * Disables sending the PairDeviceToAccount request to the service during a RegisterServicePairAccount
 * operation.  When this option is enabled, the device will perform all local operations associated
 * with registering a service, but will not request the service to add the device to the user's account.
 */
#define WEAVE_DEVICE_CONFIG_DISABLE_ACCOUNT_PAIRING 0

#define WEAVE_DEVICE_CONFIG_USE_TEST_SERIAL_NUMBER "DUMMY_SN"

/**
 * WEAVE_DEVICE_CONFIG_ENABLE_TRAIT_MANAGER
 *
 * Explicitly disable the trait manager since the application will be taking care of publishing
 * and subscribing to traits
 */
#define WEAVE_DEVICE_CONFIG_ENABLE_TRAIT_MANAGER 0

#define WEAVE_CONFIG_MAX_BINDINGS   8

#endif // WEAVE_PROJECT_CONFIG_H
