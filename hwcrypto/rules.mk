#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
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

CONSTANTS := $(LOCAL_DIR)/hwcrypto_consts.json

MODULE_SRCS := \
	$(LOCAL_DIR)/main.c \
	$(LOCAL_DIR)/hwrng_srv.c \
	$(LOCAL_DIR)/hwkey_srv.c \

ifeq (true,$(call TOBOOL,$(WITH_FAKE_HWRNG)))
MODULE_SRCS += $(LOCAL_DIR)/hwrng_srv_fake_provider.c
endif

ifeq (true,$(call TOBOOL,$(WITH_FAKE_HWKEY)))
MODULE_SRCS += $(LOCAL_DIR)/hwkey_srv_fake_provider.c
endif

MODULE_DEPS := \
	external/boringssl \
	trusty/user/base/interface/hwrng \
	trusty/user/base/interface/hwkey \
	trusty/user/base/lib/libc-trusty \
	trusty/user/base/lib/tipc \

include make/module.mk
