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

MODULE_INCLUDES +=\
	$(LOCAL_DIR)/../../include \

MODULE_SRCS += \
	$(LOCAL_DIR)/appobj.S \
	$(LOCAL_DIR)/main.c \
	$(LOCAL_DIR)/manifest.c \

MODULE_DEPS += \
	trusty/user/base/lib/libc-trusty \
	trusty/user/base/lib/tipc \
	trusty/user/base/lib/unittest \

TEST_APPS_DIR := test_apps
TEST_APPS := \
	duplicate_port \
	load_start \
	port_start \


TEST_APPS_ELFS = $(foreach TEST_APP, $(TEST_APPS), \
    $(call TOBUILDDIR, $(TEST_APPS_DIR)/$(TEST_APP)/$(TEST_APP).elf))

MODULE_SRCDEPS += $(TEST_APPS_ELFS)

MODULE_ASMFLAGS += $(foreach TEST_ELF, $(TEST_APPS_ELFS), \
    -DFILE_NAME_$(notdir $(basename $(TEST_ELF)))=$(TEST_ELF))

include make/module.mk
