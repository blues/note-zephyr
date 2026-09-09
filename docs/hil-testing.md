# Hardware-in-the-Loop Testing

`note-zephyr` is a Notecard driver, so most of what can go wrong in it goes
wrong against real hardware: I2C framing, transport timeouts, response parsing.
The build-only workflow catches none of that. The [HIL Tests][workflow]
workflow runs the module against a real Swan and a real Notecard on a
[Notestation][notestation] — a Raspberry Pi that tunnels a physical Notecard
workstation over the Blues Tailnet.

[workflow]: ../.github/workflows/hil-tests.yml
[notestation]: https://github.com/blues/notestation

## What runs

Two stages, in order, because they fail differently:

1. **blinky smoke test.** Flashes `examples/blinky` with the I2C overlay and
   waits for `Entering main loop...` on the console. blinky only reaches that
   line after its startup `hub.set` succeeds, so this one string proves the
   whole chain — reserve, flash, boot, talk to the Notecard, console capture.
   When this fails, the problem is the plumbing, not the tests.
2. **`tests/notecard` ztest suite.** The actual assertions, run under
   [twister][twister] in `--device-testing` mode.

[twister]: https://docs.zephyrproject.org/latest/develop/test/twister.html

The suite deliberately avoids anything needing cellular or Notehub
connectivity. A test that waited for a sync would fail for reasons that have
nothing to do with this module, and a flaky HIL job gets ignored, which makes
it worse than no job at all. Every case talks to the Notecard over the local
transport and asserts on the response:

| Case | What a failure means |
|---|---|
| `test_card_version` | The transport is broken. Read this one first — if it fails, the rest will too. |
| `test_consecutive_requests_are_stable` | Reads are being truncated or misaligned. Issues `card.version` three times and requires an identical answer; a bare "did it error" check would miss this. |
| `test_hub_set` | A write-path regression. Every example issues `hub.set` at startup, so this breaks all of them. |
| `test_note_add_queues_locally` | Nested request bodies are mishandled. Queues to `hil.qo` without syncing. |
| `test_unknown_request_returns_error` | Errors are being swallowed. Without this, every case above could pass against a transport that silently dropped failures. |

## Running it

The workflow is `workflow_dispatch` and nightly `schedule` only — see
[Enabling the PR trigger](#enabling-the-pr-trigger). To run it by hand, use the
Actions tab or:

```bash
gh workflow run hil-tests.yml --repo blues/note-zephyr
```

Inputs:

| Input | Default | Notes |
|---|---|---|
| `notestation_tags` | `mcu_swan mcu_debugger` | Both tags matter. `mcu_swan` says a Swan is present; `mcu_debugger` says there is an SWD probe on it, which is what flashing goes through. |
| `notestation` | *(empty)* | A specific hostname, e.g. `barcelona-notestation-1`. Overrides the tags. |
| `console_env` | `NS_HOST_MCU_USB` | Which reserved device carries the Zephyr console — see below. |
| `zephyr_version` | `v4.4.0` | Pinned rather than tracking `main`, so a HIL failure means a note-zephyr regression rather than Zephyr drift. |

## The console device

`swan_r5.dts` puts both `zephyr,console` and `zephyr,shell-uart` on `lpuart1`,
a hardware UART on the Feather header. That would be the nicer console to use —
it survives a reset and never re-enumerates — but **the Notestations do not wire
it**. `barcelona-notestation-1` exposes only `host_mcu_usb` and `notecard_usb`;
both `NS_HOST_MCU_UART0` and `NS_HOST_MCU_UART1` are empty. `note-c`'s HIL
workflow uses `host_mcu_usb` on the same station for the same reason.

So both HIL builds apply Zephyr's [`cdc-acm-console`][snippet] snippet, moving
the console onto the Swan's native USB — `required_snippets` in
`testcase.yaml` for the suite, `-S cdc-acm-console` for the blinky build. The
snippet brings in the USB device stack and initialises CDC ACM at boot, so no
application code changes.

[snippet]: https://docs.zephyrproject.org/latest/build/snippets/index.html

That buys a working console at the cost of a soft-USB device, which needs three
things to be true:

- **`--flash-before`.** Twister's default is to open the serial port *before*
  flashing, which is right for a hardware UART but wrong here: flashing tears
  the USB endpoint down underneath an open handle. This is the flicker class
  `note-c`'s workflow documents at length.
- **The flash script waits for the device to return.** Twister opens the port
  with a single un-retried `serial.Serial()` straight after the flash command
  exits, so `notestation_flash.py` polls until the reservation's symlink is back
  *and* can be opened. Existence alone is not enough — the symlink can reappear
  before the endpoint accepts an open.
- **`CONFIG_BOOT_DELAY=3000`.** Otherwise the board can emit the ztest banner
  into a port nobody is reading yet, and twister waits for output that has
  already gone.

If the selected device is empty on the reserved Notestation, the workflow fails
immediately and prints every `NS_*` device the reservation *did* expose, so a
station wired differently is a one-input fix rather than a mystery. That is much
better than the alternative: a run that reserves, flashes, then hangs on a
silent port until the job timeout.

If a Notestation ever does wire `lpuart1` through to the Pi, that is the better
console: set `console_env` to the matching `NS_HOST_MCU_UART*`, drop
`--flash-before` and the snippet, and the soft-USB machinery above becomes
unnecessary.

## Running the suite locally

You need Tailnet access and `notestation-client` on `PATH`.

```bash
# Reserve, and keep it held in the background.
notestation-client reserve --tags mcu_swan,mcu_debugger &
RESV_PID=$!
RESV_DIR=$(ls -d ~/.notestation/pid-${RESV_PID}_* | head -1)

# Build, then run against the hardware. Run these from the west topdir, not
# from the note-zephyr directory, and adjust -T to match.
west twister -p swan_r5 -T tests \
  --fixture notecard_i2c --build-only -O twister-out

export NS_HOSTNAME=$(jq -r .hostname "$RESV_DIR/reservation.json")
export CONSOLE_PORT="$RESV_DIR/host_mcu_usb"

west twister -p swan_r5 -T tests \
  --fixture notecard_i2c --test-only --no-clean -O twister-out \
  --device-testing \
  --device-serial "$CONSOLE_PORT" \
  --device-serial-baud 115200 \
  --flash-before \
  --flash-command "$PWD/scripts/hil/notestation_flash.py"

kill $RESV_PID
```

Note the `--fixture notecard_i2c`: without it twister builds the suite but
skips executing it, because `testcase.yaml` declares that fixture. Building
without the flag is a useful compile check on its own.

## How the pieces fit

Twister cannot flash a board it cannot see, and the Swan is on the far end of a
Tailnet tunnel. Two hooks bridge that:

- **`scripts/hil/notestation_flash.py`** is passed to twister's
  `--flash-command`. Twister calls it with `--build-dir`, and it hands
  `<build-dir>/zephyr/zephyr.elf` to `notestation-client flash`. It must be the
  `.elf`: `notestation-client` detects the target from the file's signature
  bytes, and host MCU flashing only accepts `.elf`/`.out`. OpenOCD ends its
  flash script with `reset run`, so the board is executing by the time twister
  starts reading.
- **`--device-serial`** points at the reservation's PTY symlink, with
  `--flash-before` so twister is not holding it open across the flash. See
  [The console device](#the-console-device) for why that combination is
  required.

`scripts/hil/console_expect.py` handles the smoke test, which is an ordinary
application rather than a test binary and so has no twister harness. It exits
non-zero on a timeout and fails fast if a `<err>` line or a Zephyr fatal shows
up before the expected string.

## Enabling the PR trigger

The workflow has no `pull_request` trigger yet. Adding one is a two-line change
to `.github/workflows/hil-tests.yml`, but do it only after a dispatch run is
green, and note what it commits to:

- Every PR takes exclusive hold of a Notestation. The `concurrency` group means
  a new push cancels its own in-flight run, but separate PRs still queue.
- Fork PRs cannot see the secrets. The job's `if:` already skips them rather
  than failing, so this is handled — but it does mean HIL results are absent on
  fork contributions.

## Credentials

Two separate things, following the pattern in
[`blues/hub`'s notestation-e2e workflow][hub-e2e] rather than the older
approach in `blues/note-c`:

[hub-e2e]: https://github.com/blues/hub/blob/master/.github/workflows/notestation-e2e.yml

| Name | Kind | Used for |
|---|---|---|
| `TAILSCALE_OAUTH_CLIENT_ID` | org secret | Joining the Blues Tailnet |
| `TAILSCALE_OAUTH_CLIENT_SECRET` | org secret | Joining the Blues Tailnet |
| `BLUES_NOTE_ZEPHYR_AUTOMATION_APP_ID` | repo **variable** | GitHub App that mints the `notestation` download token |
| `BLUES_NOTE_ZEPHYR_AUTOMATION_PRIVATE_KEY` | repo secret | Private key for that app |

The Tailscale pair is org-level, so nothing needs copying into this repo. Note
these are **not** `note-c`'s `TS_OAUTH_CLIENT_ID`/`TS_OAUTH_CLIENT_SECRET` —
those are repo-local to note-c, and using those names here yields an empty
value and an `OAuth identity empty` failure.

`notestation-client` ships from the private `blues/notestation` repo. Rather
than a long-lived PAT, the workflow mints a short-lived token with
`actions/create-github-app-token`, scoped to `owner: blues` and
`repositories: notestation`. The app id is a *variable* rather than a secret
because it is not sensitive.

For that to work, the note-zephyr automation GitHub App has to be **installed
on `blues/notestation` with contents read access**. If it is not, the
`Generate GitHub App token` step fails immediately — that is the signal to ask
whoever administers the Blues GitHub Apps to grant it, the same grant
`BLUES_HUB_AUTOMATION_APP_ID` already has.
