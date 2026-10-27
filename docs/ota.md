# OTA and recovery

Packaging and delivery are separate. This repository's build scripts perform
offline operations only. Use the separately maintained
[CMF Watch Utils](https://github.com/arnav-kr/cmf-watch-utils) stock client when
the watch runs stock firmware, or the existing Actions OTA client when it runs
this custom application. This repository intentionally does not copy device
addresses, pairing state or a one-click flash script from the experiment logs.

## Image layout

The SDK's ACTH post-build step checksums the newly compiled app. The inner AOTA
contains `app.bin` (file ID 4) and `sdfs.bin` (ID 5), and is chunk-compressed into
`TEMP.bin`. The outer AOTA targets TEMP (ID 254). A verified AGPS trailer from
stock 1.0.0.73 is appended because the stock CMF receiver requests that record.

No bootloader, recovery, parameter table, external resource or font partition is
updated. Preserving these partitions reduces scope; it does not make flashing
universally recoverable.

The independent verifier checks every nested AOTA header/data/file/XML CRC,
ACTH sum32, partition allowlist, offsets, overlaps, chunk limits and AGPS hash.
An image failing any check exits nonzero. Verification does not certify hardware
stability or guarantee that the target watch has the same partition map.

## Stock client rules

Pair freshly when needed, turn off competing phone Bluetooth connections and use
stable charger power. On Windows, stale cached services caused zero-byte failures
in earlier attempts; an uncached connection retained from pairing through OTA
succeeded. INIT2 belongs on the OTA characteristic, and the final END acknowledgement
is mandatory. Do not improvise the protocol or treat a byte count as boot proof.

The v13 baseline completed A041 success, END, and terminal progress 100. That is
transfer/commit evidence only; the subsequent boot/playback check is pending.

## Recovery limits

Keep a locally verified rollback image and its matching tooling before a device
test. Crown-held charger mode has helped recover earlier failures, but its
independence from the application and availability after all possible failures
have not been established. UART/USB recovery is not proven by this project.
