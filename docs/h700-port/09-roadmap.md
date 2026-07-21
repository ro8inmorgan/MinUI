# 09 — Release Roadmap and Gates

This document owns the H700 release lifecycle: what each stage proves, which tests
gate promotion, and what is deferred or optional. Test results are not repeated here;
every test ID points to the canonical report in
[08-testing-status.md](08-testing-status.md). Per-core detail remains in
[10-core-game-matrix.md](10-core-game-matrix.md).

## Current position

Alpha 1, Alpha 2, and Alpha 3 are complete. The port is preparing its Beta candidate.
The H700 Auto governor now permits the full in-spec 1.5 GHz range; PERF-05 hardware
validation, the planned per-model out-of-box profiles, and the candidate-specific
H700/tg5040 build and linkage checks remain.

## Stage roadmap

The Alpha rows normalize the completed bring-up history into clear stage scopes; use
these definitions when reading older alpha artifacts and commit notes.

| Stage | Purpose and issue focus | Required test areas | Status / exit |
|---|---|---|---|
| Alpha 1 — platform bring-up | Prove stock-card boot, basic display/input/audio, WiFi, and playable cores on the RG40XXV baseline | [BUILD-01](08-testing-status.md#build-and-binary-compatibility), [BOOT-01, BOOT-02](08-testing-status.md#boot-install-and-launcher), [DISP-01](08-testing-status.md#display-and-user-interface), [INPUT-01](08-testing-status.md#input-and-controllers), [AUDIO-01, AUDIO-02](08-testing-status.md#audio), [NET-01](08-testing-status.md#network-and-online-features), [GAME-01](08-testing-status.md#games-and-performance) | ✅ Complete |
| Alpha 2 — family expansion | Fix sleep/Files/UI regressions and prove real hardware deltas: RG34XXSP panel/lid/sticks, RG28XX rotation, Bootlogo sizes, and shared capability gating | [POWER-01, POWER-03](08-testing-status.md#power-sleep-and-battery), [APP-01](08-testing-status.md#applications-and-device-specific-hardware), [DISP-02, DISP-03, DISP-04, DISP-05, DISP-06, DISP-07](08-testing-status.md#display-and-user-interface), [INPUT-02, INPUT-03](08-testing-status.md#input-and-controllers), [DEV-02, DEV-03, DEV-05, DEV-06, DEV-07](08-testing-status.md#applications-and-device-specific-hardware) | ✅ Complete |
| Alpha 3 — Beta hardening | Close feature gaps and ambiguous policies: HDMI, Bluetooth controller/audio, screenshots, headphone route, MD Auto CPU, PS1 packaging, and representative core smoke | [DISP-08](08-testing-status.md#display-and-user-interface), [INPUT-05](08-testing-status.md#input-and-controllers), [AUDIO-03, AUDIO-04, AUDIO-05](08-testing-status.md#audio), [GAME-03, GAME-04, GAME-06](08-testing-status.md#games-and-performance), [PERF-04](08-testing-status.md#games-and-performance) | ✅ Complete; partial long-tail rows are classified below |
| Beta — public validation | Publish the supported H700 family build, collect broader device/firmware evidence, and fix any new critical regression | Beta entry gates and planned work below | Preparing: PERF-05, per-model defaults, and candidate checks remain |
| RC — release qualification | Freeze scope, close recovery checks, validate the packaged candidate, and accept or fix all Beta findings | RC entry gates below | Planned |
| Release — supported baseline | Promote an RC with no open release blocker and with support/known-issue documentation matching behavior | Release gates below | Planned |

## Beta entry gates

All required rows must pass on the Beta artifact. Family-level evidence is sufficient
unless the code or hardware differs.

| Gate | Required tests | Current disposition |
|---|---|---|
| Candidate build integrity | [BUILD-01, BUILD-02, BUILD-03](08-testing-status.md#build-and-binary-compatibility) | Pending only on the published-candidate run |
| Boot and shared runtime | [BOOT-01, BOOT-02](08-testing-status.md#boot-install-and-launcher), [DISP-01](08-testing-status.md#display-and-user-interface), [INPUT-01](08-testing-status.md#input-and-controllers), [AUDIO-01, AUDIO-02](08-testing-status.md#audio), [NET-01](08-testing-status.md#network-and-online-features), [GAME-01](08-testing-status.md#games-and-performance) | Satisfied |
| Power and resume | [POWER-01, POWER-02](08-testing-status.md#power-sleep-and-battery) | Satisfied |
| Hardware deltas | [DEV-01, DEV-02, DEV-03](08-testing-status.md#applications-and-device-specific-hardware) | Satisfied; DEV-04 depth is future work |
| HDMI | [DISP-08](08-testing-status.md#display-and-user-interface), [AUDIO-05](08-testing-status.md#audio) | Satisfied |
| Bluetooth audio | [AUDIO-04](08-testing-status.md#audio) | Satisfied |
| Core/performance baseline | [GAME-02, GAME-03, GAME-04](08-testing-status.md#games-and-performance), [PERF-01, PERF-04](08-testing-status.md#games-and-performance) | Satisfied |
| Auto CPU full range | [PERF-05](08-testing-status.md#games-and-performance) | High priority: implementation complete; hardware result pending |

## Beta planned work

| Work | Priority / target | Completion condition |
|---|---|---|
| Auto governor full-range scaling | High — ASAP, required for Beta | PERF-05 passes on a representative H700 device |
| Per-model out-of-box defaults | Planned for Beta | Ship tested H700-model profiles covering shader choice, emulator/core options, and relevant frontend defaults so each model starts with sensible settings |

The per-model profiles are an intentional H700-specific departure from NextUI's
otherwise barebones cross-platform defaults. Keep the overrides narrow, documented,
and tested so they improve first-run behavior without silently changing other
platforms.

Beta does not require every device or long-tail core to be tested. A new Beta finding
becomes a Beta blocker only when it causes data loss, prevents boot/gameplay on the
supported baseline, breaks a promised feature, or shows that family-level evidence
does not generalize.

## RC entry gates

| Gate | Exit condition |
|---|---|
| Beta blocker closure | Every critical/high-impact Beta regression is fixed, or the affected feature/model is explicitly removed from RC scope |
| Recovery | [BOOT-03](08-testing-status.md#boot-install-and-launcher), [ROBUST-01 and ROBUST-02](08-testing-status.md#recovery-and-robustness) pass end to end |
| Packaged build | [BUILD-01, BUILD-02, BUILD-03](08-testing-status.md#build-and-binary-compatibility) pass on the RC artifact |
| Regression smoke | [BOOT-01](08-testing-status.md#boot-install-and-launcher), [GAME-01, GAME-04](08-testing-status.md#games-and-performance), [POWER-01, POWER-02](08-testing-status.md#power-sleep-and-battery), [NET-02](08-testing-status.md#network-and-online-features), and [AUDIO-02, AUDIO-04](08-testing-status.md#audio) are rerun on a representative H700 device |
| Hardware-change targeting | Re-run the relevant DEV/INPUT/DISP row only when Beta changes touch that hardware delta |
| Documentation | Install prerequisites, supported models, and accepted issues below match the RC behavior |

## Release gates

| Gate | Exit condition |
|---|---|
| RC soak | No unresolved boot, suspend/resume, audio-route, save-state, or data-integrity regression is found during RC use |
| Final artifact | BUILD-01, BUILD-02, BUILD-03, and the RC regression smoke pass on the exact release package |
| Known issues | Every accepted partial/failing row is documented with user impact and workaround where one exists |
| Support boundary | Supported family behavior, hardware-specific exceptions, firmware prerequisites, and out-of-scope features are explicit |
| Rollback | The one-file `dmenu.bin` removal path and old-style-theme prerequisite are present in release instructions |

## Accepted behavior and non-gates

These are settled decisions, not unfinished stage work.

| Item | Test evidence | Decision |
|---|---|---|
| AirPods do not always auto-connect | [AUDIO-04](08-testing-status.md#audio) | AirPods may prefer a nearby paired iPhone/Mac. Remembered pairing plus manual Connect is the supported path |
| RG34XXSP POWER while lid is closed | [POWER-03](08-testing-status.md#power-sleep-and-battery) | Light sleep may wake; deep sleep re-sleeps while the lid remains closed |
| Charging permits light sleep only | [POWER-04](08-testing-status.md#power-sleep-and-battery) | Shared product behavior is accepted |
| FBNeo missing-BIOS hard-lock | [GAME-05](08-testing-status.md#games-and-performance) | Upstream core defect; ship as a known issue. Valid-BIOS gameplay passes |
| Clean uninstall exercise | [BOOT-04](08-testing-status.md#boot-install-and-launcher) | Not a runtime gate; deleting the only hijack file restores stock boot by design |
| Stock MU-style theme | [BOOT-05](08-testing-status.md#boot-install-and-launcher) | Install prerequisite: use old style. MU style bypasses NextUI before its code runs |
| Full matrix on every H700 model | [Test rules](08-testing-status.md#status-and-evidence-rules) | Not required without a hardware/firmware delta or contrary evidence |
| FN1/FN2/HOME and keep-awake-over-USB | [CAP-01, CAP-02](08-testing-status.md#capability-boundaries) | Unsupported RG XX/Brick-specific capabilities; not release promises |

## Deferred from Beta to RC

| Work | Test | Reason |
|---|---|---|
| Crash-loop cutoff | [BOOT-03](08-testing-status.md#boot-install-and-launcher) | The five-crash poweroff guard exists in code; inducing and observing the full sequence belongs in release qualification |
| Stock-launcher signal resilience | [ROBUST-01](08-testing-status.md#recovery-and-robustness) | The ignore-signal guard exists; end-to-end service-stop behavior belongs in release qualification |
| Dirty-card recovery | [ROBUST-02](08-testing-status.md#recovery-and-robustness) | Code exists; induced recovery test belongs in release qualification |
| Final cross-platform candidate build | [BUILD-02](08-testing-status.md#build-and-binary-compatibility) | Must be executed against each artifact being promoted, not treated as a one-time feature result |

## Future hardening

These improve confidence or breadth but do not gate the current release unless new
evidence turns one into a supported-baseline defect.

| Work | Evidence / next result needed |
|---|---|
| Overnight drain comparison | Extend [POWER-06](08-testing-status.md#power-sleep-and-battery) with `voltage_now` and a stock-OS baseline |
| Battery UI consistency | Reproduce the top-bar versus Battery-pak desynchronization noted in POWER-05 |
| Bluetooth soak | Longer suspend/resume, device-switch, and game-switch cycles beyond the passing AUDIO-04 path |
| Controller normalization | Fix and retest [INPUT-06](08-testing-status.md#input-and-controllers) across tg5040/tg5050/H700 |
| Bluetooth audio-link detection | Distinguish A2DP from controller-only ACL links before applying sample-rate policy |
| Long-tail core coverage | Expand [GAME-06](08-testing-status.md#games-and-performance) and the matrix in [10](10-core-game-matrix.md) |
| RGcubexx depth | Expand [DEV-04](08-testing-status.md#applications-and-device-specific-hardware) from community UI scaling to functional/core coverage |
| Additional model strings | Confirm RG35XX-family and RG40XXH `RGXX_MODEL` values on hardware |

## Enhancements

| Enhancement | Scope |
|---|---|
| H700 overlay packs | Ship useful overlays for 640×480, 720×480, and 720×720 logical output. Target Beta if tested packs are ready; otherwise ship after Release. This is not a Beta gate because the rendering pipeline already passes DISP-05 |
| Per-panel DisplayCal defaults | Measure panels and replace the neutral presets |
| Headphone UI | Optional HP icon and independent headphone volume; hardware routing already passes AUDIO-03 |
| Richer battery metrics | Add voltage, charge-counter, and time-to-empty data to the battery model |
| Broader per-game tuning | Extend the planned Beta per-model defaults only where a core or title demonstrably needs a narrower override |
| 720×480 Files density | Evaluate PPU 3; PPU 2 already passes APP-01 |

## Stretch goals and out-of-scope work

| Item | Classification |
|---|---|
| Broader HDMI EDID/display compatibility | Stretch: fixed-mode HDMI already passes; add fallback modes when real displays require them |
| Panel-Fix tool | Stretch feature |
| Prebake SDL2 / jammy libasound into h700-toolchain | Follow-up: dedicated image ships (`h700-toolchain`); content specialization still optional |
| Pak Store | Separate product scope; NET-04 is not exercised for this port |
| OTA updates | Separate product scope; NET-05 is not exercised for this port |

## Next actions

1. Validate PERF-05 on hardware and record the result in 08.
2. Define, test, and ship the per-model shader/emulator/frontend defaults planned for Beta.
3. Build the Beta artifact and record BUILD-01, BUILD-02, and BUILD-03 against it.
4. Publish Beta with the accepted behaviors and support boundary above.
5. Triage Beta findings against the blocker definition; add new evidence to 08 rather
   than duplicating it here.
6. Before RC, close BOOT-03 and ROBUST-01/02 and run the packaged regression smoke.
7. Promote RC to Release only after the final-artifact and documentation gates pass.
