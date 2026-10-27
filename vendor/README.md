# Local vendor inputs (not committed)

Supply these files locally:

| File | Source / role |
|---|---|
| `libbt_test.a` | Matching SDK watch sample, `prebuild/bt_production_test/libbt_test.a`; baseline link dependency |
| `sdfs.bin` | Previously working SDK/vendor SDFS; configuration, Bluetooth tables/patch and supporting data |
| `stock.bin` | JX402 stock 1.0.0.73 AOTA; used to obtain its mandatory AGPS trailer |

Baseline SDFS SHA-256:
`16bec831210ff887cfb9bab5397cd16c104524dfc8cd88b16e7721a52e2e34be`.

The required AGPS trailer is 1,242,696 bytes, SHA-256
`648b05af22f7f17cb54c314a4b7a351e947e086f35878366afb021f86b8885a4`.
The packager validates its identity. The stock application executable is not
patched or linked into our app.

The external SDK also supplies precompiled Bluetooth stack/transport libraries
and sensor algorithm libraries. Having the SDK source tree alone does not make
this a completely blob-free firmware build. Preserve each vendor's terms and
obtain the required inputs separately.
