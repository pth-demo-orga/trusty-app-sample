/*
 * Copyright (C) 2019 The Android Open Source Project
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

.macro embed_bin name, path
.section .data.loadable_apps.\name\(), "aw"
.global _bin_\name\()_begin
.type _bin_\name\()_begin, STT_OBJECT
_bin_\name\()_begin:
.incbin "\path"
.global _bin_\name\()_end
.type _bin_\name\()_end, STT_OBJECT
_bin_\name\()_end:
.endm

embed_bin load_start, FILE_NAME_load_start
embed_bin port_start, FILE_NAME_port_start
embed_bin duplicate_port, FILE_NAME_duplicate_port


