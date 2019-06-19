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
#      Travis CI build preparation script for the OpenWeave nRF52840 Lock
#      Example Application.
#

TMPDIR=${TMPDIR-/tmp}
CACHEDIR=${TRAVIS_BUILD_DIR}/cache
HASH_CMD="shasum -a 256"

FetchURL() {
    local URL="$1"
    local LOCAL_FILE_NAME=$2
    local HASH=$3

    # NOTE: 2 spaces required between hash value and file name.
    if ! (echo "${HASH}  ${LOCAL_FILE_NAME}" | ${HASH_CMD} -c --status >/dev/null 2>&1); then
        rm -f ${LOCAL_FILE_NAME}
        wget -O ${LOCAL_FILE_NAME} -nv "${URL}" || exit 1
    fi
}

# Tool download links
#
NORDIC_SDK_URL=https://www.nordicsemi.com/-/media/Software-and-other-downloads/SDKs/nRF5-SDK-for-Thread/nRF5-SDK-for-Thread-and-Zigbee/nRF5SDKforThreadandZigbeev310c7c4730.zip
NORDIC_SDK_HASH=9ba017d7e67f360855ce49b86c2a8c64f4971028e50caefad407d3e04f977162

if test "${TRAVIS_OS_NAME}" = "linux"; then

    NORDIC_COMMAND_LINE_TOOLS_URL=https://www.nordicsemi.com/-/media/Software-and-other-downloads/Desktop-software/nRF-command-line-tools/sw/Versions-9-x-x/nRF-Command-Line-Tools_9_8_1_Linux-x86_64.tar
    NORDIC_COMMAND_LINE_TOOLS_HASH=ed3eb5325f9e1dcbfc2046f3b347b7b76a802ddb31a8b113965b4097a893f6d1

    ARM_GCC_TOOLCHAIN_URL=https://developer.arm.com/-/media/Files/downloads/gnu-rm/7-2018q2/gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2
    ARM_GCC_TOOLCHAIN_HASH=bb17109f0ee697254a5d4ae6e5e01440e3ea8f0277f2e8169bf95d07c7d5fe69

elif test "${TRAVIS_OS_NAME}" = "osx"; then

    NORDIC_COMMAND_LINE_TOOLS_URL=https://www.nordicsemi.com/-/media/Software-and-other-downloads/Desktop-software/nRF-command-line-tools/sw/Versions-9-x-x/nRF-Command-Line-Tools_9_8_1_OSX.tar
    NORDIC_COMMAND_LINE_TOOLS_HASH=b4b77e4368267ba948f5bedbdc1be7699322e453c4e9f097f48763b78e192ff2

    ARM_GCC_TOOLCHAIN_URL=https://developer.arm.com/-/media/Files/downloads/gnu-rm/7-2018q2/gcc-arm-none-eabi-7-2018-q2-update-mac.tar.bz2
    ARM_GCC_TOOLCHAIN_HASH=c1c4af5226d52bd1b688cf1bd78f60eeea53b19fb337ef1df4380d752ba88759

fi


# --------------------------------------------------------------------------------

set -x

mkdir -p ${CACHEDIR}

# Install Nordic nRF52840 SDK for Thread and Zigbee
#
NORDIC_SDK_FILE_NAME=${CACHEDIR}/`basename ${NORDIC_SDK_URL}`
FetchURL "${NORDIC_SDK_URL}" ${NORDIC_SDK_FILE_NAME} ${NORDIC_SDK_HASH}
unzip -d ${TRAVIS_BUILD_DIR}/nRF5x-SDK-for-Thread-and-Zigbee -q ${NORDIC_SDK_FILE_NAME} || exit 1

# Install Nordic nRF5x Command Line Tools
#
NORDIC_COMMAND_LINE_TOOLS_FILE_NAME=${CACHEDIR}/`basename ${NORDIC_COMMAND_LINE_TOOLS_URL}`
FetchURL "${NORDIC_COMMAND_LINE_TOOLS_URL}" ${NORDIC_COMMAND_LINE_TOOLS_FILE_NAME} ${NORDIC_COMMAND_LINE_TOOLS_HASH}
mkdir ${TRAVIS_BUILD_DIR}/nRF5x-Command-Line-Tools
tar -C ${TRAVIS_BUILD_DIR}/nRF5x-Command-Line-Tools -xf ${NORDIC_COMMAND_LINE_TOOLS_FILE_NAME} || exit 1

# Install ARM GCC Toolchain
#
ARM_GCC_TOOLCHAIN_FILE_NAME=${CACHEDIR}/`basename ${ARM_GCC_TOOLCHAIN_URL}`
FetchURL "${ARM_GCC_TOOLCHAIN_URL}" ${ARM_GCC_TOOLCHAIN_FILE_NAME} ${ARM_GCC_TOOLCHAIN_HASH}
mkdir ${TRAVIS_BUILD_DIR}/arm
tar -jxf ${ARM_GCC_TOOLCHAIN_FILE_NAME} --directory ${TRAVIS_BUILD_DIR}/arm || exit 1

# Initialize and update all submodules within the example app.
#
git -C ${TRAVIS_BUILD_DIR} submodule init || exit 1
git -C ${TRAVIS_BUILD_DIR} submodule update || exit 1

set +x

# Log relevant build information.
#
echo '---------------------------------------------------------------------------'
echo 'Build Preparation Complete'
echo ''
echo "openweave-nrf52840-lock-example branch: ${TRAVIS_BRANCH}"
echo "Nordic SDK for Thread and Zigbee: ${NORDIC_SDK_URL}"
echo "Nordic nRF5x Command Line Tools: ${NORDIC_COMMAND_LINE_TOOLS_URL}"
echo "ARM GCC Toolchain: ${ARM_GCC_TOOLCHAIN_URL}"
echo 'Commit Hashes'
echo '  openweave-nrf52840-lock-example: '`git -C ${TRAVIS_BUILD_DIR} rev-parse --short HEAD`
echo '---------------------------------------------------------------------------'
