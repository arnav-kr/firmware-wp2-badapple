# Third-party provenance

| Component | Location | Provenance |
|---|---|---|
| SDK sample/framework integration | `app/sdk/`, `app/prebuild/` | Existing Actions watch SDK application copy; original notices retained |
| Board/driver adaptations | `board/`, `drivers/` | SDK source plus local stock-derived settings and fixes |
| minimp3 | `third_party/minimp3/` | lieff/minimp3, commit `ea99364f61c14656440e8d77e9c233ccf3124633`; CC0 |
| miniz 3.0.2 | `app/sdk/ui/svgrender/miniz-3.0.2/` | SDK-supplied source, upstream notices retained |
| libspng 0.7.4 | `app/sdk/ui/svgrender/libspng-0.7.4/` | SDK-supplied source, upstream notices retained |
| Zephyr/LVGL/SDK libraries | External SDK | Required separately, not vendored here |
| Video/audio | Ignored `assets/private/` | User-supplied media, not distributed here |

`docs/migration-baseline.json` records hashes and source locations of the custom
modules before this migration. It is a provenance aid, not a complete SBOM or a
claim that the upstream SDK revision is pinned.
