#!/usr/bin/env bash
set -euo pipefail
cd /workspace/JerzyMasterLabXJerzyMasterLabX
python3 -m pip install --target /workspace/tools --upgrade cmake==4.4.4 ninja==1.13.2
mkdir -p /workspace/apt/lists/partial /workspace/apt/cache/archives/partial /workspace/apt/packages
printf '%s\n' 'deb https://deb.debian.org/debian trixie main' > /workspace/apt/sources.list
apt_options=(-o Dir::State::lists=/workspace/apt/lists -o Dir::Cache=/workspace/apt/cache -o Dir::Etc::parts=- -o Dir::Etc::sourcelist=/workspace/apt/sources.list -o Dir::Etc::sourceparts=-)
apt-get "${apt_options[@]}" update
cd /workspace/apt/packages
apt-get "${apt_options[@]}" download libasound2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev libxrender-dev libxfixes-dev libfreetype-dev libfontconfig-dev libgl-dev libglx-dev x11proto-dev xorg-sgml-doctools libxau-dev libxdmcp-dev libxcb1-dev libpng-dev libbrotli-dev zlib1g-dev libbz2-dev libfreetype6 libfontconfig1 libexpat1-dev libx11-6 libbz2-1.0 zlib1g libasound2t64 libpng16-16t64 libbrotli1 libexpat1 libxinerama1 libxau6 libxfixes3 libxcursor1 libgl1 libglx0 libxext6 libxdmcp6 libxrender1 libxrandr2 libxcb1 libglvnd0
apt-get "${apt_options[@]}" download xvfb/trixie xserver-common/trixie libxfont2/trixie libxkbfile1/trixie libfontenc1/trixie x11-xkb-utils/trixie openbox/trixie libobrender32v5/trixie libobt2v5/trixie libimlib2t64/trixie libstartup-notification0/trixie libxcb-util1/trixie
for package in *.deb; do dpkg-deb -x "$package" /workspace/sysroot; done
cd /workspace/JerzyMasterLabXJerzyMasterLabX
source scripts/cloud-env.sh
"$MASTERLAB_CMAKE" -S . -B build-core -DMASTERLAB_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
"$MASTERLAB_CMAKE" --build build-core --parallel 2
"$MASTERLAB_CTEST" --test-dir build-core --output-on-failure
"$MASTERLAB_CMAKE" -S . -B build-plugin -DCMAKE_BUILD_TYPE=Release
"$MASTERLAB_CMAKE" --build build-plugin --parallel 2
"$MASTERLAB_CTEST" --test-dir build-plugin --output-on-failure
bash scripts/gui-smoke.sh
mkdir -p /workspace/tools/pluginval
curl --fail --location --max-time 60 https://github.com/Tracktion/pluginval/releases/download/v1.0.3/pluginval_Linux.zip -o /workspace/tools/pluginval/archive.zip
printf '%s\n' '69adc87d6bbb2f7224a038d8f8c5e737ea5c906ea153752b8a546b6125e0564f  /workspace/tools/pluginval/archive.zip' | sha256sum -c -
python3 - <<'PY'
import zipfile
with zipfile.ZipFile('/workspace/tools/pluginval/archive.zip') as archive:
    archive.extractall('/workspace/tools/pluginval')
PY
chmod u+x /workspace/tools/pluginval/pluginval
bash scripts/gui-smoke.sh --validate
