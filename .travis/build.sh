#!/bin/sh

#
#    Copyright 2019 Google LLC All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
#    Description:
#      Travis CI build script for the OpenWeave nRF52840 Lock Example Application.
#

# Export NRF5_SDK_ROOT variable pointing to the nRF5x SDK for Thread and Zigbee.
export NRF5_SDK_ROOT=${TRAVIS_BUILD_DIR}/nRF5x-SDK-for-Thread-and-Zigbee

# Export NRF5_TOOLS_ROOT variable pointing to the nRF5x command line tools.
export NRF5_TOOLS_ROOT=${TRAVIS_BUILD_DIR}/nRF5x-Command-Line-Tools

# Export GNU_INSTALL_ROOT variable pointing to the ARM GCC tool chain.
export GNU_INSTALL_ROOT=${TRAVIS_BUILD_DIR}/arm/gcc-arm-none-eabi-7-2018-q2-update/bin/

# Build the example application.
make -C ${TRAVIS_BUILD_DIR} DEVICE_FIRMWARE_REVISION=1.0d${TRAVIS_BUILD_NUMBER} || exit 1
