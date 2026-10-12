#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
test_dir="$(mktemp -d /tmp/lab1-tests.XXXXXX)"
trap 'rm -rf -- "$test_dir"' EXIT
common=(-std=c11 -Wall -Wextra -Werror -ITests/fake_hal -IApp
        -ILibraries/leds -ILibraries/button -ILibraries/seven_segment
        -ILibraries/infrared)
app=(App/lab_app.c Libraries/leds/led_effects.c
     Libraries/button/button.c Libraries/seven_segment/7seg.c)
cc "${common[@]}" Tests/test_lab.c "${app[@]}" -o "$test_dir/lab"
cc "${common[@]}" Tests/test_ir.c Libraries/infrared/irda.c -o "$test_dir/ir"
cc "${common[@]}" -DLAB_EXERCISE=4 Tests/test_remote.c "${app[@]}" \
   Libraries/infrared/irda.c -o "$test_dir/remote"
"$test_dir/lab"
"$test_dir/ir"
"$test_dir/remote"
