# Madeira upstream IPA source

This branch builds from [`willfaust/Madeira` commit `8c050d03f4d89096e1e2e2c8bb44479fffd86619`](https://github.com/willfaust/Madeira/commit/8c050d03f4d89096e1e2e2c8bb44479fffd86619), committed 2026-09-25. The FEX, Wine, and DXMT gitlinks are kept at the exact upstream revisions.

The `.github/workflows/build.yml` recipe is a downstream build helper carried from the Build 79 lineage build. The produced IPA is ad hoc signed and needs suitable signing for device installation. It is a CI build of upstream source, not a release published by the upstream project.
