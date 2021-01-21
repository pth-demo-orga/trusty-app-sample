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

BOOT_START_APP  := $(abspath $(BUILDDIR)/../boot-start-srv/boot-start-srv.app)
NEVER_START_APP := $(abspath $(BUILDDIR)/../never-start-srv/never-start-srv.app)
PORT_START_APP  := $(abspath $(BUILDDIR)/../port-start-srv/port-start-srv.app)
RESTART_APP     := $(abspath $(BUILDDIR)/../restart-srv/restart-srv.app)

MODULE_SRCS += \
	$(LOCAL_DIR)/main.c \
	$(LOCAL_DIR)/apps.S \

MODULE_DEPS += \
	trusty/user/base/lib/libc-trusty \
	trusty/user/base/lib/tipc \
	trusty/user/base/lib/unittest \
	trusty/user/base/interface/apploader \

MODULE_ASMFLAGS += \
	-DBOOT_START_APP=\"$(BOOT_START_APP)\" \
	-DNEVER_START_APP=\"$(NEVER_START_APP)\" \
	-DPORT_START_APP=\"$(PORT_START_APP)\" \
	-DRESTART_APP=\"$(RESTART_APP)\" \

MODULE_INCLUDES += \
	$(LOCAL_DIR)/../include \

MODULE_SRCDEPS += \
	$(BOOT_START_APP) \
	$(NEVER_START_APP) \
	$(PORT_START_APP) \
	$(RESTART_APP) \

include make/module.mk
