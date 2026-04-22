#!/bin/bash

# 1. Prepare credentials
sudo -v

# 2. Get current TTY number
CURRENT_VT=$(fgconsole)

# 3. Launch Math Engine (Foreground)
echo "Launching Math Visualizer..."

# 4. Start recording in background AFTER a small delay
# This allows the engine to become DRM Master first.
(
  sleep 3
  echo "Recording started..."
  sudo ffmpeg -y -vaapi_device /dev/dri/renderD128 -f kmsgrab -device /dev/dri/card1 -i - -vf 'hwmap=derive_device=vaapi,scale_vaapi=format=nv12' -c:v h264_vaapi -f matroska output.mkv < /dev/null > /dev/null 2>&1
) &
FFMPEG_PID=$!

sudo ./math_visualizer

# 5. Cleanup
echo "Exiting..."
sudo killall -INT ffmpeg > /dev/null 2>&1
wait $FFMPEG_PID 2>/dev/null

# 6. FORCE Terminal Restore
# This performs the "Manual TTY switch" automatically.
sudo chvt 1
sleep 0.1
sudo chvt $CURRENT_VT

echo "Terminal restored. Video saved to output.mkv"
