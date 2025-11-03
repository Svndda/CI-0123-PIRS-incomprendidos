#!/usr/bin/env bash
set -euo pipefail

# Test script: compile Arduino_Node, run a UDP listener, run node in simulate mode,
# capture packets and assert decoded values are within expected ranges.

PROJ_DIR="/home/YiYhuan/Desktop/CI-0123-PIRS-incomprendidos"
SRC="$PROJ_DIR/SafeSpace/server/src/nodes/Arduino/Arduino_Node.cpp"
OUT="/tmp/Arduino_Node_test"
RECV="/tmp/udp_test_recv.txt"

MASTER_IP="${1:-127.0.0.1}"
PORT="${2:-9999}"

CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -pthread"

echo "Compiling $SRC (with main wrapper)..."
g++ $CXXFLAGS "$SRC" SafeSpace/server/src/nodes/Arduino/Arduino_Node_main.cpp -o "$OUT"

rm -f "$RECV"

# Start listener in background; it will write decoded packets to $RECV
python3 -u - <<PY >"$RECV" 2>&1 &
import socket,sys
s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("0.0.0.0", int($PORT)))
# receive up to 3 packets or timeout
s.settimeout(6.0)
for i in range(3):
  try:
    data,addr=s.recvfrom(4096)
  except socket.timeout:
    break
  if len(data) < 13:
    print('short', len(data), data.hex())
    continue
  msgId = data[0]
  t = int.from_bytes(data[1:3], 'big', signed=True)
  h = int.from_bytes(data[3:5], 'big', signed=True)
  dist = int.from_bytes(data[5:7], 'big', signed=True)
  press = int.from_bytes(data[7:11], 'big', signed=True)
  alt = int.from_bytes(data[11:13], 'big', signed=True)
  print(f"hex={data.hex()} msgId=0x{msgId:02x} temp_x100={t} hum_x100={h} dist_x100={dist} press_pa={press} alt_x100={alt} from={addr}")
sys.stdout.flush()
PY
listener_pid=$!

echo "Listener started (pid=$listener_pid), waiting 0.5s before starting node..."
sleep 0.5

# Run node in simulate mode in binary format; it will run until we kill it
"$OUT" "$MASTER_IP" "$PORT" simulate binary &
node_pid=$!

# Wait for listener to finish (it will exit after timeout or after receiving packets)
wait $listener_pid || true

# Kill node (it may still be running)
kill $node_pid 2>/dev/null || true
wait $node_pid 2>/dev/null || true

if [ ! -s "$RECV" ]; then
  echo "Test failed: no packets captured; see $RECV for listener output"
  exit 2
fi

echo "Captured packets:" 
cat "$RECV"

# Basic assertions: at least one decoded line with temp/hum in expected ranges
ok=0
while read -r line; do
  # expect lines like: hex=42234... msgId=0x42 temp_x100=2345 hum_x100=5678 from=('127.0...')
  if echo "$line" | grep -q "msgId=0x42"; then
    t=$(echo "$line" | sed -n 's/.*temp_x100=\([0-9-]*\).*/\1/p')
    h=$(echo "$line" | sed -n 's/.*hum_x100=\([0-9-]*\).*/\1/p')
    if [ -n "$t" ] && [ -n "$h" ]; then
      # check reasonable ranges for simulated values
      if [ "$t" -ge 2000 ] && [ "$t" -le 2800 ] && [ "$h" -ge 4000 ] && [ "$h" -le 7000 ]; then
        ok=1
        break
      fi
    fi
  fi
done < "$RECV"

if [ "$ok" -eq 1 ]; then
  echo "Test PASSED: received simulated packets with expected ranges."
  exit 0
else
  echo "Test FAILED: packets received but values out of expected ranges or no valid msgId."
  exit 3
fi
