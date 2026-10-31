#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/tests
flags=(-std=c11 -Wall -Wextra -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer
       -ITests/fake_hal -IApp -ILibraries/rtc -ILibraries/rfid)
cc "${flags[@]}" Tests/test_logic.c App/access.c App/console.c Libraries/rtc/ds1307.c -o build/tests/test_logic
build/tests/test_logic
cc "${flags[@]}" Tests/test_rfid.c Libraries/rfid/rc522.c -o build/tests/test_rfid
build/tests/test_rfid
