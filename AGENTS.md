# Working on this project

- Keep changes inside this project and build output folders; treat the external SDK as read-only.
- Build/package/test commands are offline. Device writes and new firmware flashes need explicit user authorization.
- Do not commit vendor binaries, generated media, firmware images, device addresses, serial numbers, pairing/session keys or raw BLE logs.
- Preserve the OTA protocol and app/SDFS-only packaging scope. Do not resize partitions or update boot/recovery as routine cleanup.
- Separate source/build/transfer checks from on-watch verification. Do not describe the v13 display fix as proven on hardware.
- Read README.md and docs/architecture.md before editing. Custom code is in app/src; retained SDK integration is in app/sdk.
- Host tests run through tests/CMakeLists.txt. ARM builds use scripts/build.ps1; scripts/verify.py independently checks an OTA.
- Preserve upstream notices. There is no blanket license covering all SDK-derived files.
