# Copyright (C) 2020 The Android Open Source Project
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

ifneq ($(TRUSTY_USER_ARCH),arm64)
$(error Only arm64 is supported.)
endif

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MANIFEST := $(LOCAL_DIR)/manifest.json

MODULE_SRCS += \
	$(LOCAL_DIR)/main.c \

MODULE_DEPS += \
	trusty/user/base/lib/unittest \

#BUILD_FROM_SOURCE := true

ifeq (true,$(call TOBOOL,$(BUILD_FROM_SOURCE)))
# Build library from source.
MODULE_DEPS += \
	$(LOCAL_DIR)/lib \

else
# Include headers needed to call into prebuilt library.
MODULE_INCLUDES += \
	$(LOCAL_DIR)/prebuilts/include \

# Link in the prebuilt library. This prebuilt was built from source in lib/.
# To generate it, set BUILD_FROM_SOURCE variable above. The archive can then be
# found at:
# build-root/build-<build_target>/user_tasks/<path_to_app>/<path_to_lib>.mod.a
MODULE_EXTRA_ARCHIVES += \
	$(LOCAL_DIR)/prebuilts/arch/$(ARCH)/lib.mod.a \

# Also link in dependencies of the prebuilt.
MODULE_DEPS += \
	trusty/user/base/lib/libc-trusty \

endif

include make/module.mk
