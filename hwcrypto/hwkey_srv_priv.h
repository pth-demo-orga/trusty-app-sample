/*
 * Copyright (C) 2016 The Android Open Source Project
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
#pragma once

#include <lk/compiler.h>
#include <sys/types.h>
#include <uapi/trusty_uuid.h>

struct hwkey_keyslot {
    const char* key_id;
    const uuid_t* uuid;
    const void* priv;
    uint32_t (*handler)(const struct hwkey_keyslot* slot,
                        uint8_t* kbuf,
                        size_t kbuf_len,
                        size_t* klen);
};

/**
 * struct hwkey_derived_keyslot_data - data for a keyslot which derives its key
 * by decrypting a fixed key
 *
 * This slot data is used by hwkey_derived_keyslot_handler() which will decrypt
 * the encrypted data using the key from retriever().
 *
 * @encrypted_key_data:
 *     Block-sized IV followed by encrypted key data
 */
struct hwkey_derived_keyslot_data {
    const uint8_t* encrypted_key_data;
    const unsigned int* encrypted_key_size_ptr;
    const void* priv;
    uint32_t (*retriever)(const void* priv,
                          uint8_t* kbuf,
                          size_t kbuf_len,
                          size_t* klen);
};

/*
 * Max size (in bytes) of a key returned by &struct
 * hwkey_derived_keyslot_data.retriever
 */
#define HWKEY_DERIVED_KEY_MAX_SIZE 32

__BEGIN_CDECLS

/**
 * hwkey_derived_keyslot_handler() - Return a slot-specific key using the key
 * data from hwkey_derived_keyslot_data
 *
 * Some devices may store a shared encryption key in hardware. However, we do
 * not want to alllow multiple clients to directly use this key, as they would
 * then be able to decrypt each other's data. To solve this, we want to be able
 * to derive unique, client-specific keys from the shared encryption key.
 *
 * To use this handler for key derivation from a common shared key, the
 * encrypting entity should generate a unique, random key for a particular
 * client, then encrypt that unique key using the common shared key resulting in
 * a wrapped, client-specific key. This wrapped key can then be safely embedded
 * in the hwkey service in the &struct
 * hwkey_derived_keyslot_data.encrypted_key_data field and will only be
 * accessible using the shared key which is retrieved via the &struct
 * hwkey_derived_keyslot_data.retriever callback.
 */
uint32_t hwkey_derived_keyslot_handler(const struct hwkey_keyslot* slot,
                                       uint8_t* kbuf,
                                       size_t kbuf_len,
                                       size_t* klen);

void hwkey_init_srv_provider(void);

void hwkey_install_keys(const struct hwkey_keyslot* keys, unsigned int kcnt);

int hwkey_start_service(void);

uint32_t derive_key_v1(const uuid_t* uuid,
                       const uint8_t* ikm_data,
                       size_t ikm_len,
                       uint8_t* key_data,
                       size_t* key_len);

__END_CDECLS
