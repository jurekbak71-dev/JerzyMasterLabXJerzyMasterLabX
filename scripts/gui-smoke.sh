#!/usr/bin/env bash
set -euo pipefail
cd /workspace/JerzyMasterLabXJerzyMasterLabX
source scripts/cloud-env.sh
# Serialize desktop tests and their shared log files.
exec 9>build-plugin/gui-tests.lock
flock --exclusive 9
mkdir -p /tmp/.X11-unix
export XDG_CACHE_HOME=/workspace/cache
export XDG_DATA_DIRS=/workspace/sysroot/usr/share:/usr/share
export DISPLAY=:98
mkdir -p "$XDG_CACHE_HOME/fontconfig"
export LD_LIBRARY_PATH=/workspace/sysroot/usr/lib/x86_64-linux-gnu
/workspace/sysroot/usr/bin/Xvfb :98 -screen 0 2000x1400x24 -nolisten tcp -ac > build-plugin/xvfb.log 2>&1 &
masterlab_xvfb_pid=$!
masterlab_wm_pid=
cleanup() {
 if [ -n "$masterlab_wm_pid" ]; then
  kill "$masterlab_wm_pid" 2>/dev/null || true
  wait "$masterlab_wm_pid" 2>/dev/null || true
 fi
 kill "$masterlab_xvfb_pid" 2>/dev/null || true
 wait "$masterlab_xvfb_pid" 2>/dev/null || true
}
trap cleanup EXIT
for attempt in {1..50}; do
 kill -0 "$masterlab_xvfb_pid" 2>/dev/null || { cat build-plugin/xvfb.log; exit 1; }
 if [ -S /tmp/.X11-unix/X98 ]; then break; fi
 sleep 0.1
done
/workspace/sysroot/usr/bin/openbox --sm-disable --config-file /workspace/sysroot/etc/xdg/openbox/rc.xml > build-plugin/openbox.log 2>&1 &
masterlab_wm_pid=$!
python3 scripts/wm-ready.py
kill -0 "$masterlab_wm_pid"
if [ "${1:-}" = --validate ]; then
 /workspace/tools/pluginval/pluginval --strictness-level 5 --validate "$PWD/build-plugin/JerzyMasterLabX_artefacts/Release/VST3/JerzyMasterLabX.vst3" > build-plugin/pluginval.log 2>&1
else
 ./build-plugin/plugin_tests --gui
fi
