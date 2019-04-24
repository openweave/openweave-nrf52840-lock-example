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
#      Travis CI build script for nRF52840 integration builds.
#

# Setup build artifacts only 
# NRF5_SDK_ROOT variable pointing to the nRF5x SDK for Thread and Zigbee.
NRF5_SDK_ROOT=${TRAVIS_BUILD_DIR}/nRF5x-SDK-for-Thread-and-Zigbee

BUILD_ARTIFACTS_DIR_NAME=build_artifacts
BUILD_ARTIFACTS_DIR_PATH=${TRAVIS_BUILD_DIR}/${BUILD_ARTIFACTS_DIR_NAME}

# Create a new directory for build artifacts
mkdir -p ${BUILD_ARTIFACTS_DIR_PATH} || exit 1

# Copy build artifacts into the directory created in the previous step
cp ${TRAVIS_BUILD_DIR}/build/openweave-nrf52840-lock-example.hex ${BUILD_ARTIFACTS_DIR_PATH}
cp ${TRAVIS_BUILD_DIR}/build/openweave-nrf52840-lock-example.map ${BUILD_ARTIFACTS_DIR_PATH}
cp ${TRAVIS_BUILD_DIR}/build/openweave-nrf52840-lock-example.log ${BUILD_ARTIFACTS_DIR_PATH}
cp $(NRF5_SDK_ROOT)/components/softdevice/s140/hex/s140_nrf52_6.1.0_softdevice.hex ${BUILD_ARTIFACTS_DIR_PATH}

# TODO: Also copy the build log

