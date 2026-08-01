# Changelog

## v0.2.0 - 2026-07-31

- Added an installation mode select using the official LD2460 `0x09` command
  and a reusable ceiling-mode package that selects `top` after startup.
- Added the universal SmartHomeShop ceiling and coordinate profiles.

## v0.1.1 - 2026-05-31

- Defines package UART buses as lists so the package merges cleanly with firmware that already has another UART bus.
- Corrects the UltimateSensor Mini V2 LD2460 example pin mapping to GPIO4/GPIO5.

## v0.1.0 - 2026-04-22

- Initial release of the HLK-LD2460 external component.
- Exposes presence, target count, target coordinates, distance and angle entities.
- Uses concise entity names intended to be displayed under each Home Assistant device.
- Published Home Assistant state updates only when target state changes.
- Supports current ESPHome releases, including ESPHome 2026.4.
- Adds SmartHomeShop package URLs, tracking package IDs, product examples, and CI validation.
