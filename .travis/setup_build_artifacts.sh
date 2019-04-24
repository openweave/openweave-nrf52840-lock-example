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

# Create a new directory for build artifacts
mkdir -p ${TRAVIS_BUILD_DIR}/build_artifacts || exit 1

# Copy build artifacts into the directory created in the previous step
cp ${TRAVIS_BUILD_DIR}/build/openweave-nrf52840-lock-example.hex ${TRAVIS_BUILD_DIR}/build_artifacts
cp ${TRAVIS_BUILD_DIR}/build/openweave-nrf52840-lock-example.map ${TRAVIS_BUILD_DIR}/build_artifacts
cp $(NRF5_SDK_ROOT)/components/softdevice/s140/hex/s140_nrf52_6.1.0_softdevice.hex ${TRAVIS_BUILD_DIR}/build_artifacts

# Display all the copied build artifacts
ll ${TRAVIS_BUILD_DIR}/build_artifacts




