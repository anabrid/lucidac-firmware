#!/bin/bash
#
# Downloading and Staging the static file assets for the LucidacWebServer
# =======================================================================
#
# This bash script does the following:
#
# 1. Downloads the most recent Single Page Application Build from gitlab
# 2. Unpacks it, loops over every file, collects file information
# 3. Sets up the file /lib/communication/src/web/assets.cpp in a way that
#    GCC can compile it within platformio without any further preperation.
#    During compilation, it will efficiently embed the downloaded and unpacked
#    straight into the firmware executable. However, this is already after
#    the runtime/scope of this bash script.
#
# Why bash?
#
#    Was easy. Could also easily move to Python, which would increase the
#    compatibility to Windows (bash runs fine on Linux, the CI, Mac OS X
#    but not so fine on Windows).
#
# Is this part of the PlatformIO/Scons build process?
#
#    No. Integration could be possible but we certainly would not like to
#    query a new SPA build every time the compilation starts, during
#    development. Since the C++ code fails gracefully if no static files
#    are included, it is on you to run this script BEFORE compiling the
#    firmware in order to INCLUDE the javascript application within the
#    firmware.
#
# Assumptions:
#
#    Gitlab CI builds a single ZIP file with static files from a primarily
#    dynamic king kong of application (they call it svelte). The Firmware
#    does not want to do anything with that. We are only interested in the
#    outcome. Therefore one ZIP which contains files. Nothing more.
#
#    The ZIP can contain any subdirectory structure. It will be mapped on
#    the static served path.
#
#    Static serving in the firmware is very basic (i.e. no directory listings)
#
#    This bash script can do gzipping at runtime (to reduce firmware pressure)
#    but that's it. Nothing fancy.
#
#    Note to inter-Git-linkage: Since we are interested in build outputs, git
#    submodules are not an option. Instead, we use a token which is directly
#    stored here. No reason to avoid storing a secret in the repository, it can
#    be reset any time.
#

set -e
shopt -s globstar # bash

cd "$(dirname "$0")" # go where the script is stored

# Using the token `artifact-download` with value `glpat-n6Ys-Wr2pQn8Qo-_dU_V`
# (only read permission, nothing critical):

curl -L --header "PRIVATE-TOKEN: glpat-n6Ys-Wr2pQn8Qo-_dU_V" \
    "https://lab.analogparadigm.com/api/v4/projects/257/jobs/artifacts/main/download?job=build_for_teensy" \
     > dist.zip

rm -fr dist
unzip -q dist.zip
cd dist

# get rid of any sourcemaps, if present.
rm -f **/*map*

assets="../../src/web/assets.cpp"

asset_structures=$(mktemp)
asset_http_headers=$(mktemp)
asset_extern_symbols=$(mktemp)
asset_assembler_includes=$(mktemp)

echo "const web::StaticFile assets[] = {" > $asset_structures

for fn in * */**; do
[ -d "$fn" ] && continue
gzip -9q22k "$fn"
lastmod_unixtime=$(stat -c%Y "${fn}")

# educated guess how the linker rewrites the filename.
# TODO probably caveat: Subdirectory will not be included into the linker symbol but is here.
linkerfn=$(echo "${fn}" | sed 's#[/.-]#_#g')

echo "extern uint8_t _binary_${linkerfn}_start[], _binary_${linkerfn}_end[];"  >> $asset_extern_symbols

# File path relative to platformio project root.
# or just use a full absolute path
path_relative_to_project_root="$(realpath "$fn")"


# Correct section is .progmem
# cf. imxrt1062_t41.ld linker script
# https://www.devever.net/~hl/incbin
# https://dox.ipxe.org/embedded_8c_source.html
cat << ASM >> $asset_assembler_includes
__asm__(
 ".section \".progmem.webfiles\", \"a\", %progbits\n"
 ".global _binary_${linkerfn}_start\n"
 ".type   _binary_${linkerfn}_start,%object\n"
 "_binary_${linkerfn}_start:\n"
 ".incbin \"${path_relative_to_project_root}.gz\"\n"
 "_binary_${linkerfn}_end:\n"
 ".previous\n"
);
ASM


mime_type="$(file --brief --mime-type "${fn}")"

# For CSS and JS files, the correct mime type is important.
if   [[ "$fn" == *.js  ]]; then mime_type="application/javascript"; # is correctly detected anyway
elif [[ "$fn" == *.css ]]; then mime_type="text/css";               # this fix is important
fi

cat << HTTP_HEADER > "${fn}.http"
{ "Content-Type", "$mime_type" },
{ "Last-Modified", "$(TZ=GMT date -R -d @$lastmod_unixtime | sed 's/+0000/GMT/')" },
{ "Date", "$(TZ=GMT date -R -d @$lastmod_unixtime | sed 's/+0000/GMT/')" },
{ "Cache-Control", "max-age=604800" },
{ "ETag", "$(md5sum "${fn}.gz" | head -c8)" },
HTTP_HEADER

# use this fields only when having GZIP encoding turned on:

cat << HTTP_HEADER >> "${fn}.http"
{ "Content-Length", "$(stat -c%s "${fn}.gz")" },
{ "Content-Encoding", "gzip" },
HTTP_HEADER


#prefab_http_header_name="prefab_http_headers_for_${linkerfn}";
#echo "const char ${prefab_http_header_name}[] = " >> $asset_http_headers
#cat "${fn}.http" | awk -v q='"' '{print q$0"\\n"q}' >> $asset_http_headers
#echo "; /* end of ${fn} */" >> $asset_http_headers

echo "{ \"${fn}\", _binary_${linkerfn}_start, (uint32_t)(_binary_${linkerfn}_end - _binary_${linkerfn}_start), { $(cat "${fn}.http") }, $lastmod_unixtime }," >> $asset_structures

done

echo "};" >> $asset_structures

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
 
#include "web/assets.h"

CPP_HEADER

cat $asset_extern_symbols $asset_assembler_includes $asset_http_headers $asset_structures >> $assets

rm $asset_extern_symbols $asset_assembler_includes $asset_http_headers $asset_structures

cat << CPP_FOOTER >> $assets

web::StaticAttic::StaticAttic() : files(assets), number_of_files(sizeof(assets) / sizeof(assets[0])) {}

CPP_FOOTER


# For testing, can do this, should run without error
#g++ $assets && rm $assets.gch

echo "Number of asset files:  $(find | grep gz | wc -l)"
echo "Total asset size:       $(find | grep gz | xargs du -hcs | awk '{print $1}' | tail -n1)"
