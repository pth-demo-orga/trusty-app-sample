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

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MANIFEST := $(LOCAL_DIR)/manifest.json

CONSTANTS := $(LOCAL_DIR)/../include/app_mgmt_port_consts.json

MODULE_INCLUDES += \
	$(LOCAL_DIR)/../include \

MODULE_SRCS += \
	$(LOCAL_DIR)/main.c \

MODULE_DEPS += \
	trusty/user/base/lib/libc-trusty \

ifneq ($(strip $(APPLOADER_SIGN_UNLOCKED_KEY_ID)),)
# Sign this application with the dev key for testing
ifneq ($(APPLOADER_SIGN_PRIVATE_KEY_$(APPLOADER_SIGN_UNLOCKED_KEY_ID)_FILE),)
APPLOADER_SIGN_KEY_ID_FOR_$(MODULE) := $(APPLOADER_SIGN_UNLOCKED_KEY_ID)
endif
endif

include make/module.mk
