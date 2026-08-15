# 09 — Release Roadmap and Gates

This document owns the H700 release lifecycle: what each stage proves, which tests
gate promotion, and what is deferred or optional. Test results are not repeated here;
every test ID points to the canonical report in
[08-testing-status.md](08-testing-status.md). Per-core detail remains in
[10-core-game-matrix.md](10-core-game-matrix.md).

## Current position

Alpha 1–3, Beta (`h700-beta1`), and RC1–RC8 are complete. **`h700-rc8` is the golden
master** about to be opened as the PR for official merge upstream.

RC8 was not a feature delta over RC7: it is a rebuild that rebased the H700 port onto
the exact upstream commit used by NextUI **v6.14.0**, deliberately excluding a later
RetroAchievements-related change that prevented many achievements from triggering.
minarch was rebuilt on that base; the H700 platform work itself is otherwise the same
lineage as RC7.

Remaining work for this document is gate/evidence hygiene for the merge PR (open
rows in 08, any unsettled RC/Release checklist items), not further Alpha/Beta bring-up.

## Stage roadmap

The Alpha rows normalize the completed bring-up history into clear stage scopes; use
these definitions when reading older alpha artifacts and commit notes.

| Stage | Purpose and issue focus | Required test areas | Status / exit |
|---|---|---|---|
| Alpha 1 — platform bring-up | Prove stock-card boot, basic display/input/audio, WiFi, and playable cores on the RG40XXV baseline | [BUILD-01](08-testing-status.md#build-and-binary-compatibility), [BOOT-01, BOOT-02](08-testing-status.md#boot-install-and-launcher), [DISP-01](08-testing-status.md#display-and-user-interface), [INPUT-01](08-testing-status.md#input-and-controllers), [AUDIO-01, AUDIO-02](08-testing-status.md#audio), [NET-01](08-testing-status.md#network-and-online-features), [GAME-01](08-testing-status.md#games-and-performance) | ✅ Complete |
| Alpha 2 — family expansion | Fix sleep/Files/UI regressions and prove real hardware deltas: RG34XXSP panel/lid/sticks, RG28XX rotation, Bootlogo sizes, and shared capability gating | [POWER-01, POWER-03](08-testing-status.md#power-sleep-and-battery), [APP-01](08-testing-status.md#applications-and-device-specific-hardware), [DISP-02, DISP-03, DISP-04, DISP-05, DISP-06, DISP-07](08-testing-status.md#display-and-user-interface), [INPUT-02, INPUT-03](08-testing-status.md#input-and-controllers), [DEV-02, DEV-03, DEV-05, DEV-06, DEV-07](08-testing-status.md#applications-and-device-specific-hardware) | ✅ Complete |
| Alpha 3 — Beta hardening | Close feature gaps and ambiguous policies: HDMI, Bluetooth controller/audio, screenshots, headphone route, MD Auto CPU, PS1 packaging, and representative core smoke | [DISP-08](08-testing-status.md#display-and-user-interface), [INPUT-05](08-testing-status.md#input-and-controllers), [AUDIO-03, AUDIO-04, AUDIO-05](08-testing-status.md#audio), [GAME-03, GAME-04, GAME-06](08-testing-status.md#games-and-performance), [PERF-04](08-testing-status.md#games-and-performance) | ✅ Complete; partial long-tail rows are classified below |
| Beta — public validation | Publish the supported H700 family build, collect broader device/firmware evidence, and fix any new critical regression | Beta entry gates below | ✅ Complete (`h700-beta1`) |
| RC — release qualification | Freeze scope, close recovery checks, validate the packaged candidate, and accept or fix all Beta findings | RC entry gates below | ✅ Complete through `h700-rc8` (GM) |
| Release — supported baseline | Promote an RC with no open release blocker and with support/known-issue documentation matching behavior | Release gates below | ✅ Release gates satisfied for `h700-rc8`; open the upstream merge PR |

## Beta entry gates

All required rows must pass on the Beta artifact. Family-level evidence is sufficient
unless the code or hardware differs.

| Gate | Required tests | Current disposition |
|---|---|---|
| Candidate build integrity | [BUILD-01, BUILD-02, BUILD-03, BUILD-04](08-testing-status.md#build-and-binary-compatibility) | Satisfied on `h700-rc8`; BUILD-02 notes a tg5040 LedControl persistence report ([#23](https://github.com/pvaibhav/NextUI/issues/23)) |
| Boot and shared runtime | [BOOT-01, BOOT-02](08-testing-status.md#boot-install-and-launcher), [DISP-01](08-testing-status.md#display-and-user-interface), [INPUT-01](08-testing-status.md#input-and-controllers), [AUDIO-01, AUDIO-02](08-testing-status.md#audio), [NET-01](08-testing-status.md#network-and-online-features), [GAME-01](08-testing-status.md#games-and-performance) | Satisfied |
| Power and resume | [POWER-01, POWER-02](08-testing-status.md#power-sleep-and-battery) | Satisfied |
| Hardware deltas | [DEV-01, DEV-02, DEV-03, DEV-04](08-testing-status.md#applications-and-device-specific-hardware) | Satisfied (DEV-04 user-confirmed on cube) |
| HDMI | [DISP-08](08-testing-status.md#display-and-user-interface), [AUDIO-05](08-testing-status.md#audio) | Satisfied |
| Bluetooth audio | [AUDIO-04](08-testing-status.md#audio) | Satisfied |
| Core/performance baseline | [GAME-02, GAME-03, GAME-04](08-testing-status.md#games-and-performance), [PERF-01, PERF-04](08-testing-status.md#games-and-performance) | Satisfied |
| Auto CPU full range | [PERF-05](08-testing-status.md#games-and-performance) | Satisfied |

## Beta planned work

| Work | Priority / target | Completion condition |
|---|---|---|
| ~~Auto governor full-range scaling~~ | Was required for Beta | ✅ [PERF-05](08-testing-status.md#games-and-performance) passed on hardware |
| Per-model out-of-box defaults | **Post-merge enhancement** (not a `h700-rc8` merge blocker) | Ship tested H700-model profiles covering shader choice, emulator/core options, and relevant frontend defaults so each model starts with sensible settings |

The per-model profiles remain an intentional H700-specific departure from NextUI's
otherwise barebones cross-platform defaults. Keep the overrides narrow, documented,
and tested so they improve first-run behavior without silently changing other
platforms. They are **not** required to open or merge the upstream PR.

Beta does not require every device or long-tail core to be tested. A new Beta finding
becomes a Beta blocker only when it causes data loss, prevents boot/gameplay on the
supported baseline, breaks a promised feature, or shows that family-level evidence
does not generalize.

## RC entry gates

| Gate | Exit condition |
|---|---|
| Beta blocker closure | Every critical/high-impact Beta regression is fixed, or the affected feature/model is explicitly removed from RC scope |
| Recovery | — | [BOOT-03](08-testing-status.md#boot-install-and-launcher), [ROBUST-01](08-testing-status.md#recovery-and-robustness), and [ROBUST-02](08-testing-status.md#recovery-and-robustness) are accepted non-gates (code present; induced E2E not required for `h700-rc8`) |
| Packaged build | [BUILD-01, BUILD-02, BUILD-03, BUILD-04](08-testing-status.md#build-and-binary-compatibility) pass on the RC artifact |
| Regression smoke | [BOOT-01](08-testing-status.md#boot-install-and-launcher), [GAME-01, GAME-04](08-testing-status.md#games-and-performance), [POWER-01, POWER-02](08-testing-status.md#power-sleep-and-battery), [NET-02](08-testing-status.md#network-and-online-features), and [AUDIO-02, AUDIO-04](08-testing-status.md#audio) are rerun on a representative H700 device |
| Hardware-change targeting | Re-run the relevant DEV/INPUT/DISP row only when Beta changes touch that hardware delta |
| Documentation | Install prerequisites, supported models, and accepted issues below match the RC behavior |

## Release gates

| Gate | Exit condition | Disposition (`h700-rc8`) |
|---|---|---|
| RC soak | No unresolved boot, suspend/resume, audio-route, save-state, or data-integrity regression is found during RC use | ✅ Satisfied |
| Final artifact | BUILD-01, BUILD-02, BUILD-03, BUILD-04, and the RC regression smoke pass on the exact release package | ✅ Satisfied on `h700-rc8`. AUDIO-04 remains Pass; one report of no automatic BT re-pair after sleep/wake is expected-not-to-work and filed for triage ([#20](https://github.com/pvaibhav/NextUI/issues/20)) |
| Known issues | Every accepted partial/failing row is documented with user impact and workaround where one exists | ✅ Documented. Includes accepted known bug [LED-01b](08-testing-status.md#capability-boundaries) (dual-bank left LED; no maintainer test device — upstream merge call). Other non-blockers: INPUT-06, GAME-05, GAME-06, [#20](https://github.com/pvaibhav/NextUI/issues/20), [#23](https://github.com/pvaibhav/NextUI/issues/23) |
| Support boundary | Supported family behavior, hardware-specific exceptions, firmware prerequisites, and out-of-scope features are explicit | ✅ Satisfied as the docs stand (`README` install/theme notes, accepted non-gates, stretch/out-of-scope, and 08/10 known partials) |
| Rollback | The one-file `dmenu.bin` removal path and old-style-theme prerequisite are present in release instructions | ✅ Satisfied (`skeleton/BASE/README.txt`) |

## Accepted behavior and non-gates

These are settled decisions, not unfinished stage work.

| Item | Test evidence | Decision |
|---|---|---|
| AirPods do not always auto-connect | [AUDIO-04](08-testing-status.md#audio) | AirPods may prefer a nearby paired iPhone/Mac. Remembered pairing plus manual Connect is the supported path |
| No automatic BT audio re-pair after sleep/wake | [AUDIO-04](08-testing-status.md#audio) / [#20](https://github.com/pvaibhav/NextUI/issues/20) | Not a supported promise; manual Connect remains the path. Filed for triage, not a `h700-rc8` Release blocker |
| RG34XXSP POWER while lid is closed | [POWER-03](08-testing-status.md#power-sleep-and-battery) | Light sleep may wake; deep sleep re-sleeps while the lid remains closed |
| Charging permits light sleep only | [POWER-04](08-testing-status.md#power-sleep-and-battery) | Shared product behavior is accepted |
| FBNeo missing-BIOS hard-lock | [GAME-05](08-testing-status.md#games-and-performance) | Upstream core defect; ship as a known issue. Valid-BIOS gameplay passes |
| Long-tail EXTRAS core smoke incomplete | [GAME-06](08-testing-status.md#games-and-performance) | Maintainer covered A2600 / mGBA / SMS / Pico-8; remaining systems in [10](10-core-game-matrix.md) stay untested by the maintainer with no specific user bug reports — known, not a merge blocker |
| Two-bank RGB left stick dead | [LED-01b](08-testing-status.md#capability-boundaries) | **Accepted known bug** for the `h700-rc8` PR: dual-bank models report only the right LED changeable. Maintainer has no dual-bank test device. Upstream may merge with this open or wait for a fix; not silently treated as Pass |
| Clean uninstall exercise | [BOOT-04](08-testing-status.md#boot-install-and-launcher) | Not a runtime gate; deleting the only hijack file restores stock boot by design |
| Stock MU-style theme | [BOOT-05](08-testing-status.md#boot-install-and-launcher) | Install prerequisite: use old style. MU style bypasses NextUI before its code runs |
| Full matrix on every H700 model | [Test rules](08-testing-status.md#status-and-evidence-rules) | Not required without a hardware/firmware delta or contrary evidence |
| FN1/FN2/HOME and keep-awake-over-USB | [CAP-01, CAP-02](08-testing-status.md#capability-boundaries) | Unsupported RG XX/Brick-specific capabilities; not release promises |
| Crash-loop cutoff exercise | [BOOT-03](08-testing-status.md#boot-install-and-launcher) | Guard exists in the launcher; inducing the five-crash poweroff path is not a Release blocker for `h700-rc8` |
| Stock-launcher SIGUSR1 resilience | [ROBUST-01](08-testing-status.md#recovery-and-robustness) | Shim ignores USR1 by design; untested end to end and not a Release blocker for `h700-rc8` |
| Dirty-card recovery exercise | [ROBUST-02](08-testing-status.md#recovery-and-robustness) | Boot fsck/repair path exists; untested end to end and not a Release blocker for `h700-rc8` |

## Deferred from Beta to RC

All former recovery deferrals ([BOOT-03](08-testing-status.md#boot-install-and-launcher),
[ROBUST-01](08-testing-status.md#recovery-and-robustness),
[ROBUST-02](08-testing-status.md#recovery-and-robustness)) and the cross-platform
build check ([BUILD-02](08-testing-status.md#build-and-binary-compatibility)) are
resolved for `h700-rc8`: BUILD-02 passed; the recovery rows are accepted non-gates.
See [Accepted behavior and non-gates](#accepted-behavior-and-non-gates).

## Future hardening

These improve confidence or breadth but do not gate the current release unless new
evidence turns one into a supported-baseline defect.

| Work | Evidence / next result needed |
|---|---|
| Battery UI consistency | Reproduce the top-bar versus Battery-pak desynchronization noted in POWER-05 |
| Bluetooth soak | Longer suspend/resume, device-switch, and game-switch cycles beyond the passing AUDIO-04 path; triage [#20](https://github.com/pvaibhav/NextUI/issues/20) (no auto re-pair after sleep) |
| Controller normalization | Fix and retest [INPUT-06](08-testing-status.md#input-and-controllers) across tg5040/tg5050/H700 |
| TrimUI LedControl persistence | [BUILD-02](08-testing-status.md#build-and-binary-compatibility) / [#23](https://github.com/pvaibhav/NextUI/issues/23) — settings do not stick on tg5040 with the shared `h700-rc8` lineage; not an H700 LED failure |
| Bluetooth audio-link detection | Distinguish A2DP from controller-only ACL links before applying sample-rate policy |
| Long-tail core coverage | Expand [GAME-06](08-testing-status.md#games-and-performance) and the matrix in [10](10-core-game-matrix.md) — known incomplete, not a merge blocker |
| RGcubexx deeper functional/core walk | [DEV-04](08-testing-status.md#applications-and-device-specific-hardware) UI/10-row layout is Pass; optional broader core coverage on cube remains post-merge |
| Two-bank LED bank order | [LED-01b](08-testing-status.md#capability-boundaries) — accepted known bug (left LED unchangeable on dual-bank reports); fix when a dual-bank device is available or a remote repro lands. Upstream may hold the merge on this |
| Additional model strings | Confirm remaining RG35XX-family `RGXX_MODEL` values on hardware (`RG40xxH` / `RGSP` / etc. already confirmed from firmware) |

## Enhancements

| Enhancement | Scope |
|---|---|
| H700 overlay packs | Ship useful overlays for 640×480, 720×480, and 720×720 logical output. Target Beta if tested packs are ready; otherwise ship after Release. This is not a Beta gate because the rendering pipeline already passes DISP-05 |
| Per-panel DisplayCal defaults | Partly done: RG28XX, RG34XXSP, RG SP, and RG40XXV ship calibrated enabled presets. Remaining H700 models stay neutral until measured |
| Headphone UI | Optional HP icon and independent headphone volume; hardware routing already passes AUDIO-03 |
| Richer battery metrics | Add voltage, charge-counter, and time-to-empty data to the battery model |
| Broader per-game tuning | Extend the planned post-merge per-model defaults only where a core or title demonstrably needs a narrower override |
| Per-model out-of-box defaults | Post-merge: shader / emulator / frontend defaults per H700 model (not a `h700-rc8` merge blocker) |
| 720×480 Files density | Evaluate PPU 3; PPU 2 already passes APP-01 |

## Stretch goals and out-of-scope work

| Item | Classification |
|---|---|
| Broader HDMI EDID/display compatibility | Stretch: fixed-mode HDMI already passes; add fallback modes when real displays require them |
| Panel-Fix tool | Stretch feature |
| Jammy-matched libasound / richer H700 sysroot | Optional: SDL2 already prebaked; BlueZ not in image |
| Pak Store | Separate product scope; NET-04 is not exercised for this port |
| OTA updates | Separate product scope; NET-05 is not exercised for this port |

## Next actions

1. Open the upstream merge PR from the `h700-rc8` golden master (rebased on the
   NextUI v6.14.0 commit so RetroAchievements behave correctly).
2. Keep post-merge follow-ups in Future hardening / Enhancements (per-model defaults,
   long-tail cores, [#20](https://github.com/pvaibhav/NextUI/issues/20),
   [#23](https://github.com/pvaibhav/NextUI/issues/23), remaining LED fleet gaps).
3. After merge, promote documentation and tags to the published Release baseline.
