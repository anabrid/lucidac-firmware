#!/bin/bash

# Assets and subdirectories:
#  Should make sure at vite build time that we don't use any subdirectories.
#  This simplifies things.

set -e

# Using the token `artifact-download` with value `glpat-n6Ys-Wr2pQn8Qo-_dU_V` (only read permission, nothing critical):

#curl -L --header "PRIVATE-TOKEN: glpat-n6Ys-Wr2pQn8Qo-_dU_V" "https://lab.analogparadigm.com/api/v4/projects/257/jobs/artifacts/main/download?job=build_static_assets"  > public.zip

rm -fr public; unzip -q public.zip; cd public

shopt -s globstar # bash

assets="../assets.h"
asset_http_headers=$(mktemp)

cat << CPP_HEADER > $assets
/*
 * This header file is auto-generated and allows for including static files
 * into the firmware, for serving them over the web.
 * It uses objcopy (GNU compiler specific) in order to speed up compilation,
 * skipping the C compiler phase.
 *
 * This file is supposed to be included exactly once where needed and thus
 * behaves as a CPP file.
 */

struct static_file {
  const char *filename;
  struct linker_addresses { uint32_t *start, *end, *size } content;
  uint32_t lastmod; // C Unix timestamp for cache control
};


static_file assets[] = {
CPP_HEADER

echo "const char pregenerated_http_headers[][] = {" > $asset_http_headers

for fn in * */**; do
[ -d "$fn" ] && continue
gzip -9q22k "$fn"
lastmod_unixtime=$(stat -c%Y "${fn}")

# educated guess how the linker rewrites the filename.
# TODO probably caveat: Subdirectory will not be included into the linker symbol but is here.
linkerfn=$(echo "${fn}" | sed 's#[/.-]#_#g')

echo "{ \"${fn}\", { _binary_${linkerfn}_start, _binary_${linkerfn}_end, _binary_${linkerfn}_size }, $lastmod_unixtime }," >> $assets

cat << HTTP_HEADER > "${fn}.http"
HTTP/1.1 200 OK
Content-Type: $(file --brief --mime-type "${fn}")
Content-Length: $(stat -c%s "${fn}.gz")
Content-Encoding: gzip
Last-Modified: $(TZ=GMT date -R -d @$lastmod_unixtime | sed 's/+0000/GMT/')
Date: $(TZ=GMT date -R -d @$lastmod_unixtime | sed 's/+0000/GMT/')
Cache-Control: max-age=604800
ETag: $(md5sum "${fn}.gz" | head -c8)
X-Reply-Type: Pregenerated Headers at PIO compile time
HTTP_HEADER

echo "/* $fn */" >> $asset_http_headers
cat "${fn}.http" | awk -v q='"' '{print q$0q}' >> $asset_http_headers
echo "," >> $asset_http_headers

#echo -e "\n" >> "$prep_http" # empty line seperating header and content
#cat "${fn}.gz" >> "$prep_http"

done

echo "};" >> $assets
echo "};" >> $asset_http_headers

cat $asset_http_headers >> $assets
rm  $asset_http_headers

