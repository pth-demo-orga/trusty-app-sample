# Copyright (C) 2019 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

TRUSTY_ALL_USER_TASKS += \
	trusty/user/app/sample/app-mgmt-test/boot-start-srv \
	trusty/user/app/sample/app-mgmt-test/client\
	trusty/user/app/sample/app-mgmt-test/never-start-srv \
	trusty/user/app/sample/app-mgmt-test/port-start-srv \
	trusty/user/app/sample/app-mgmt-test/restart-srv \
	trusty/user/app/sample/hwcrypto-unittest \
	trusty/user/app/sample/ipc-unittest/main \
	trusty/user/app/sample/ipc-unittest/srv \
	trusty/user/app/sample/manifest-test \
	trusty/user/app/sample/libcxx-test \
	trusty/user/app/sample/memref-test \
	trusty/user/app/sample/memref-test/lender \
	trusty/user/app/sample/timer \
	trusty/user/app/sample/uirq-unittest \
	trusty/user/app/sample/spi/swspi-srv \
	trusty/user/app/sample/spi/swspi-test \

TRUSTY_LOADABLE_USER_TASKS += \
