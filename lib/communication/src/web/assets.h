// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <cstdint> // uint32_t
#include <stddef.h> // size_t
#include <string.h> // strcmp

namespace web {

  /**
   * Static web server file assets are compiled into the firmware code
   * as binary objects.
   * 
   * For ease of the build system, we use an inline assembler `incbin`
   * command. This way, the C compiler does not need to preprocess the
   * amount of static data and compilation does not slow down.
   * 
   * See also https://www.devever.net/~hl/incbin and https://dox.ipxe.org/embedded_8c_source.html
   * for where this approach stems from.
   * 
   * The assets.cpp file is compiled by a shell script in the /lib/communication/assets
   * directory. The output of that file is not part of the git repository,
   * since the actual assets are also not part of the firmware git repository.
   * 
   * Instead, a fallback code is put there, to allow compiling without any assets.
   * 
   * Technically, the asset structure works as a array of structures but the API
   * is that of a lookup by filename.
   * 
   */
  struct StaticFile {
    const char *filename;
    const uint8_t *start; ///< begin of content in memory, determined by linker symbol
    const uint32_t size;  ///< size in bytes; determined by linker symbol

    static constexpr int max_headers = 10;
    /// up to 10 (unstructured) HTTP Headers, for instance
    /// Content-Type, Content-Encoding, ETag, etc.
    struct HttpHeader {
      const char* name;
      const char* value;
    } headers[max_headers];

    const uint32_t lastmod; ///< C Unix timestamp for cache control
  };

  // Get access to a table of static files, as defined by a proper implementation
  // in assetsp.cpp
  struct StaticAttic {
    const StaticFile *files;
    const size_t number_of_files;
    StaticAttic();

    /// Lookup StaticFile by name. Code also demonstrates how to traverse attic.
    /// Returns NULL if not found.
    const StaticFile* get_by_filename(const char* filename) {
      for(size_t i=0; i<number_of_files; i++)
        if(0 == strcmp(filename, files[i].filename)) return files+i;
      return NULL;
    }
  };
} // namespace web