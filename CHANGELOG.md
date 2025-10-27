# StreamUP Record Chapter Manager - Changelog

---

## v1.2.0 (27 Oct '25)
**Patch Focus:** Export formats & workflow polish
- Added Final Cut Pro XML and Premiere Pro XML export options with NTSC handling and marker metadata
- Added Edit Decision List (EDL) export for DaVinci Resolve workflows
- Added a chapter history sidebar
- Fixed recording timestamp drift and ensured export files close cleanly when recordings stop
- Refined the dock UI, including a dedicated settings icon and spacing tweaks
- Added en-GB locale entries and translated the new export controls

## v1.1.0 (20 Jul '24)
**Patch Focus:** Scene collection reliability & incremental naming
- Added optional incremental chapter naming and re-enabled StreamUP Vertical support
- Added dedicated Aitum Vertical chapter marker integration
- Fixed duplicated hotkeys when changing scene collections

## v1.0.2 (04 Jul '24)
**Patch Focus:** Live websocket events
- Added obs-websocket emitter support for chapter markers

## v1.0.1 (24 Jun '24)
**Patch Focus:** Quality-of-life fixes
- Added a support link, improved label layout, and corrected OBS module text
- Prevented export files from being created when their toggles are disabled
- Fixed build configuration issues across platforms

## v1.0.0 (19 Jun '24)
**Initial Release**
Core functionality for recording chapter markers during OBS recordings, exporting chapter lists, and managing annotations within a dedicated dock.
