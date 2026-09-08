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
| `console_env` | `NS_HOST_MCU_UART0` | Which reserved device carries the Zephyr console — see below. |
| `zephyr_version` | `v4.4.0` | Pinned rather than tracking `main`, so a HIL failure means a note-zephyr regression rather than Zephyr drift. |

## The console device

`swan_r5.dts` puts both `zephyr,console` and `zephyr,shell-uart` on `lpuart1`,
which is a hardware UART on the Feather header — **not** the Swan's native USB.
Which `NS_HOST_MCU_UART*` that lands on depends on how the Notestation is
wired, and neither the notestation repo nor this one records it, hence the
`console_env` input.

A hardware UART is the right choice here regardless of which one it is: it
survives a reset and never re-enumerates. The Swan's native USB re-enumerates
after every flash, and `note-c`'s HIL workflow carries three paragraphs about
the symlink flicker that caused.

If the chosen device is empty on the reserved Notestation, the workflow fails
immediately and prints every `NS_*` device the reservation *did* expose. Re-run
with `console_env` set to whichever one is wired to `lpuart1`. That is much
better than the alternative: a run that reserves, flashes, and then hangs on a
silent port until the job timeout.

## Running the suite locally

You need Tailnet access and `notestation-client` on `PATH`.

```bash
# Reserve, and keep it held in the background.
notestation-client reserve --tags mcu_swan,mcu_debugger &
RESV_PID=$!
RESV_DIR=$(ls -d ~/.notestation/pid-${RESV_PID}_* | head -1)

# Build, then run against the hardware.
"$ZEPHYR_BASE/scripts/twister" -p swan_r5 -T tests \
  --fixture notecard_i2c --build-only -O twister-out

NS_HOSTNAME=$(jq -r .hostname "$RESV_DIR/reservation.json") \
"$ZEPHYR_BASE/scripts/twister" -p swan_r5 -T tests \
  --fixture notecard_i2c --test-only --no-clean -O twister-out \
  --device-testing \
  --device-serial "$RESV_DIR/host_mcu_uart0" \
  --device-serial-baud 115200 \
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
- **`--device-serial`** points at the reservation's PTY symlink. Twister opens
  it *before* flashing (its `--flash-before` default is off), so the ztest
  banner is captured even though the board reboots as part of flashing.

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

## Required secrets

The workflow needs three secrets that **`blues/note-zephyr` does not currently
have at repository level**:

| Secret | Used for |
|---|---|
| `TS_OAUTH_CLIENT_ID` | Joining the Blues Tailnet |
| `TS_OAUTH_CLIENT_SECRET` | Joining the Blues Tailnet |
| `NOTESTATION_RELEASE_DOWNLOAD_TOKEN` | Downloading `notestation-client` from `blues/notestation` releases |

`blues/note-c` holds all three as repository secrets. The notestation-actions
README says the Tailscale pair is set org-wide, which would make them available
here automatically — if that is the case, nothing needs doing. Confirm before
the first run; a missing secret shows up as a Tailscale step failure.
