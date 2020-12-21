/*
 * Copyright 2020, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define TLOG_TAG "secure_fb_impl"

#include <lib/secure_dpu/secure_dpu.h>
#include <lib/secure_fb/srv/dev.h>
#include <lib/secure_fb/srv/srv.h>
#include <lib/tipc/tipc.h>
#include <lk/err_ptr.h>
#include <lk/macros.h>
#include <memref.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <trusty_ipc.h>
#include <trusty_log.h>

#include <tuple>

#define PAGE_SIZE() (getauxval(AT_PAGESZ))

static constexpr const uint32_t kDeviceWidth = 400;
static constexpr const uint32_t kDeviceHeight = 800;
static constexpr const uint32_t kFbCount = 1;
static constexpr const uint32_t kFbId = 0xdeadbeef;

static handle_t secure_dpu_handle = INVALID_IPC_HANDLE;

class SecureFbMockImpl {
private:
    struct FbDbEntry {
        secure_fb_info fb_info;
        handle_t handle;
        ptrdiff_t offset;
    };

    FbDbEntry fb_db_[kFbCount];

public:
    ~SecureFbMockImpl() {
        if (secure_dpu_free_buffer(secure_dpu_handle,
                                   (void*)fb_db_[0].fb_info.buffer) < 0) {
            TLOGE("Failed to free framebuffer\n");
        }
        if (secure_dpu_stop_secure_display(secure_dpu_handle) < 0) {
            TLOGE("Failed to stop secure_display\n");
        }
    }

    int Init(uint32_t width, uint32_t height) {
        if (secure_dpu_start_secure_display(secure_dpu_handle) < 0) {
            TLOGE("Failed to start secure_display\n");
            return SECURE_FB_ERROR_UNINITIALIZED;
        }

        uint32_t fb_size =
                round_up(sizeof(uint32_t) * width * height, PAGE_SIZE());

        void* fb_base;
        size_t buffer_len = (size_t)fb_size;
        if (secure_dpu_allocate_buffer(secure_dpu_handle,
                                       &fb_base, &buffer_len) < 0
                                       || buffer_len < fb_size) {
            TLOGE("Failed to allocate framebuffer of size: %u\n", fb_size);
            return SECURE_FB_ERROR_MEMORY_ALLOCATION;
        }
        fb_size = buffer_len;

        /*
         * Create a handle for the buffer by which it can be passed to the TUI
         * app for rendering.
         */
        static int handle =
                memref_create(fb_base, fb_size, PROT_READ | PROT_WRITE);
        if (handle < 0) {
            TLOGE("Failed to create memref (%d)\n", handle);
            free(fb_base);
            return SECURE_FB_ERROR_SHARED_MEMORY;
        }

        fb_db_[0] = {
                .fb_info =
                        {
                                .buffer = (uint8_t*)fb_base,
                                .size = fb_size,
                                .pixel_stride = 4,
                                .line_stride = 4 * width,
                                .width = width,
                                .height = height,
                                .pixel_format = TTUI_PF_RGBA8,
                        },
                .handle = handle,
        };

        return SECURE_FB_ERROR_OK;
    }

    int GetFbs(struct secure_fb_impl_buffers* buffers) {
        *buffers = {
                .num_fbs = 1,
                .fbs[0] =
                        {
                                .buffer_id = kFbId,
                                .handle_index = 0,
                                .fb_info = fb_db_[0].fb_info,
                        },
                .num_handles = 1,
                .handles[0] = fb_db_[0].handle,
        };
        return SECURE_FB_ERROR_OK;
    }

    int Display(uint32_t buffer_id) {
        if (buffer_id != kFbId) {
            return SECURE_FB_ERROR_INVALID_REQUEST;
        }

        /* This is a no-op in the mock case. */
        return SECURE_FB_ERROR_OK;
    }
};

secure_fb_handle_t secure_fb_impl_init() {
    auto sfb = new SecureFbMockImpl();
    sfb->Init(kDeviceWidth, kDeviceHeight);
    return sfb;
}

int secure_fb_impl_get_fbs(secure_fb_handle_t sfb_handle,
                           struct secure_fb_impl_buffers* buffers) {
    SecureFbMockImpl* sfb = reinterpret_cast<SecureFbMockImpl*>(sfb_handle);
    return sfb->GetFbs(buffers);
}

int secure_fb_impl_display_fb(secure_fb_handle_t sfb_handle,
                              uint32_t buffer_id) {
    SecureFbMockImpl* sfb = reinterpret_cast<SecureFbMockImpl*>(sfb_handle);
    return sfb->Display(buffer_id);
}

int secure_fb_impl_release(secure_fb_handle_t sfb_handle) {
    SecureFbMockImpl* sfb = reinterpret_cast<SecureFbMockImpl*>(sfb_handle);
    delete sfb;
    return SECURE_FB_ERROR_OK;
}

int main(void) {
    int rc;
    struct tipc_hset* hset;

    hset = tipc_hset_create();
    if (IS_ERR(hset)) {
        TLOGE("failed (%d) to create handle set\n", PTR_ERR(hset));
        return PTR_ERR(hset);
    }

    rc = add_secure_dpu_service(hset, &secure_dpu_handle);
    if (rc != NO_ERROR) {
        TLOGE("failed (%d) to initialize secure_dpu mock service\n", rc);
        return rc;
    }

    rc = add_secure_fb_service(hset);
    if (rc != NO_ERROR) {
        TLOGE("failed (%d) to initialize secure_fb mock service\n", rc);
        return rc;
    }

    return tipc_run_event_loop(hset);
}
