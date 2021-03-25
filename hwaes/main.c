/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define TLOG_TAG "hwaes_srv"

#include <assert.h>
#include <lib/hwaes_server/hwaes_server.h>
#include <lib/tipc/tipc_srv.h>
#include <lk/err_ptr.h>
#include <stdlib.h>
#include <string.h>
#include <trusty_log.h>
#include <uapi/err.h>

#include <openssl/evp.h>

static EVP_CIPHER_CTX* cipher_ctx;

static void crypt_init(void) {
    assert(!cipher_ctx);

    cipher_ctx = EVP_CIPHER_CTX_new();
    assert(cipher_ctx);
}

static void crypt_shutdown(void) {
    EVP_CIPHER_CTX_free(cipher_ctx);
    cipher_ctx = NULL;
}

static uint32_t hwaes_check_arg_helper(size_t len, const uint8_t* data_ptr) {
    if (len == 0 || data_ptr == NULL) {
        return HWAES_ERR_INVALID_ARGS;
    }
    return HWAES_NO_ERROR;
}

static uint32_t hwaes_check_arg_in(const struct hwaes_arg_in* arg) {
    return hwaes_check_arg_helper(arg->len, arg->data_ptr);
}

static uint32_t hwaes_check_arg_out(const struct hwaes_arg_out* arg) {
    return hwaes_check_arg_helper(arg->len, arg->data_ptr);
}

uint32_t hwaes_aes_op(const struct hwaes_aes_op_args* args) {
    int evp_ret;
    uint32_t rc;
    const EVP_CIPHER* cipher;
    int out_data_size;

    if (args->mode != HWAES_CBC_MODE) {
        TLOGE("the mode is not implemented yet\n");
        return HWAES_ERR_NOT_IMPLEMENTED;
    }

    if (args->padding != HWAES_NO_PADDING) {
        TLOGE("the padding type is not implemented yet\n");
        return HWAES_ERR_NOT_IMPLEMENTED;
    }

    rc = hwaes_check_arg_in(&args->key);
    if (rc != HWAES_NO_ERROR) {
        TLOGE("key argument is missing\n");
        return rc;
    }

    rc = hwaes_check_arg_in(&args->text_in);
    if (rc != HWAES_NO_ERROR) {
        TLOGE("text_in argument is missing\n");
        return rc;
    }

    rc = hwaes_check_arg_out(&args->text_out);
    if (rc != HWAES_NO_ERROR) {
        TLOGE("text_out argument is missing\n");
        return rc;
    }

    /*
     * The current implementation does not support padding.
     * So the size of input buffer is the same as output buffer.
     */
    if (args->text_in.len != args->text_out.len) {
        TLOGE("text_in_len (%zd) is not equal to text_out_len (%zd)\n",
              args->text_in.len, args->text_out.len);
        return HWAES_ERR_INVALID_ARGS;
    }

    switch (args->key.len) {
    case 16:
        cipher = EVP_aes_128_cbc();
        break;
    case 32:
        cipher = EVP_aes_256_cbc();
        break;
    default:
        TLOGE("invalid key length: (%zd)\n", args->key.len);
        return HWAES_ERR_INVALID_ARGS;
    }

    assert(cipher_ctx);
    EVP_CIPHER_CTX_reset(cipher_ctx);

    evp_ret = EVP_CipherInit_ex(cipher_ctx, cipher, NULL, NULL, NULL,
                                args->encrypt);
    if (!evp_ret) {
        TLOGE("EVP_CipherInit_ex failed\n");
        return HWAES_ERR_GENERIC;
    }

    if (args->text_in.len % EVP_CIPHER_CTX_block_size(cipher_ctx)) {
        TLOGE("text_in_len (%zd) is not block aligned\n", args->text_in.len);
        return HWAES_ERR_INVALID_ARGS;
    }

    if (EVP_CIPHER_CTX_iv_length(cipher_ctx) != args->iv.len) {
        TLOGE("invalid iv length: (%zd)\n", args->iv.len);
        return HWAES_ERR_INVALID_ARGS;
    }

    evp_ret = EVP_CipherInit_ex(cipher_ctx, cipher, NULL, args->key.data_ptr,
                                args->iv.data_ptr, args->encrypt);
    if (!evp_ret) {
        TLOGE("EVP_CipherInit_ex failed\n");
        return HWAES_ERR_GENERIC;
    }

    evp_ret = EVP_CIPHER_CTX_set_padding(cipher_ctx, 0);
    if (!evp_ret) {
        TLOGE("EVP_CIPHER_CTX_set_padding failed\n");
        return HWAES_ERR_GENERIC;
    }

    evp_ret = EVP_CipherUpdate(cipher_ctx, args->text_out.data_ptr,
                               &out_data_size, args->text_in.data_ptr,
                               args->text_in.len);
    if (!evp_ret) {
        TLOGE("EVP_CipherUpdate failed\n");
        return HWAES_ERR_GENERIC;
    }

    /*
     * The assert fails if the memory corruption happens.
     */
    assert(out_data_size == (int)args->text_out.len);

    /*
     * Currently we don't support padding.
     */
    evp_ret = EVP_CipherFinal_ex(cipher_ctx, NULL, &out_data_size);
    if (!evp_ret) {
        TLOGE("EVP_CipherFinal_ex failed\n");
        return HWAES_ERR_GENERIC;
    }

    return HWAES_NO_ERROR;
}

int main(void) {
    int rc;
    struct tipc_hset* hset;

    hset = tipc_hset_create();
    if (IS_ERR(hset)) {
        TLOGE("failed (%d) to create handle set\n", PTR_ERR(hset));
        return EXIT_FAILURE;
    }

    rc = add_hwaes_service(hset);
    if (rc != NO_ERROR) {
        TLOGE("failed (%d) to initialize hwaes service\n", rc);
        return EXIT_FAILURE;
    }

    crypt_init();
    rc = tipc_run_event_loop(hset);

    TLOGE("hwaes server going down: (%d)\n", rc);
    crypt_shutdown();
    if (rc != NO_ERROR) {
        return EXIT_FAILURE;
    }
    EXIT_SUCCESS;
}
