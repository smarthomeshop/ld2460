# ESPHome HLK-LD2460

ESPHome external component and reusable packages for the Hi-Link HLK-LD2460
multi-target radar module, maintained for SmartHomeShop firmware projects.

The component reads the documented LD2460 UART2 reporting protocol and exposes
decoded target data to Home Assistant. Detection radius, zones, sensitivity and
firmware updates can stay in the HLK Radar Tool app; the module stores those
settings itself.

## Repository Structure

```text
components/ld2460/   ESPHome external component implementation
ld2460.yaml          Reusable generic ESPHome package
tracking-ld2460.yaml Reusable SmartHomeShop tracking package with stable IDs
examples/            Product integration examples
tests/               Local validation fixture
example.yaml         Minimal copy-paste usage example
CHANGELOG.md         Release notes
```

The ESPHome component name is `ld2460`, so the package internally configures an
`ld2460:` block. UART is only the transport layer.

## Installation

Copy this into an existing ESPHome device config and adjust the pins for your
board:

```yaml
substitutions:
  ld2460_tx_pin: GPIO16 # ESP TX -> LD2460 Rx2 / RX
  ld2460_rx_pin: GPIO17 # ESP RX <- LD2460 Tx2 / TX

packages:
  ld2460:
    url: https://github.com/smarthomeshop/ld2460
    ref: v0.1.0
    files:
      - ld2460.yaml
    refresh: 1d
```

Pinning the package to a Git tag is recommended. See [CHANGELOG.md](CHANGELOG.md)
for released versions. `refresh` controls how often ESPHome checks whether the
referenced package source should be refreshed locally.

Your device config still owns the normal ESPHome device setup: `esphome:`,
`esp32:`/`esp8266:`, `wifi:`, `api:`, `ota:` and `logger:`. This package only
adds the LD2460 external component, UART bus and entities.

## Wiring

Use the LD2460 user UART, not the debug/programming pads.

| LD2460   | ESPHome board  |
| -------- | -------------- |
| Tx2 / TX | ESP RX pin     |
| Rx2 / RX | ESP TX pin     |
| GND      | GND            |
| 5V       | 5V power input |

Do not connect the LD2460 `3.3V`/`VDD33` pin when powering the module from 5V.

For a Seeed Studio XIAO ESP32-C6 example:

| LD2460   | XIAO ESP32-C6 |
| -------- | ------------- |
| Tx2 / TX | D7 / GPIO17   |
| Rx2 / RX | D6 / GPIO16   |

## Exposed Entities

- Presence
- Target Count
- Target summary text
- Target 1-5 X position
- Target 1-5 Y position
- Target 1-5 distance
- Target 1-5 angle
- Diagnostic raw UART frame
- Diagnostic UART byte counter
- Diagnostic firmware and installation mode text sensors

Home Assistant state updates are published only when the target state changes
and are additionally limited by `ld2460_publish_interval`.

## Substitutions

| Substitution                        | Default                                       | Description                                         |
| ----------------------------------- | --------------------------------------------- | --------------------------------------------------- |
| `ld2460_package_version`            | `v0.1.0`                                      | Package release marker                              |
| `ld2460_tx_pin`                     | `GPIO16`                                      | ESP TX pin connected to LD2460 Rx2                  |
| `ld2460_rx_pin`                     | `GPIO17`                                      | ESP RX pin connected to LD2460 Tx2                  |
| `ld2460_bus_id`                     | `ld2460_bus`                                  | ESPHome UART bus id                                 |
| `ld2460_component_id`               | `ld2460_radar`                                | ESPHome LD2460 component id                         |
| `ld2460_publish_interval`           | `500ms`                                       | Minimum interval between changed HA state publishes |
| `ld2460_report_log_interval`        | `1s`                                          | Interval for readable target report log lines       |
| `ld2460_external_components_source` | `github://smarthomeshop/ld2460@v0.1.0`        | External component source                           |

## Protocol Notes

The LD2460 live report frame contains target coordinates only: X and Y in
0.1 m steps. Distance and angle are derived by this component.

Useful references:

- [HLK-LD2460 product page](https://www.hlktech.net/index.php?id=1335)
- [HLK-LD2460 manual mirror](https://revspace.nl/images/9/9d/HLK-LD2460_2T4R_Multi_Target_Trajectory_Tracking_Module_Manual_V1.1.pdf)
- [HLK-LD2460 serial protocol PDF](https://storage.googleapis.com/mauser-public-images/prod_description_document%2F2025%2F260%2Fc4fbc62952a566f34adad3f4ce506cfa_hlk-ld2460_serial_port_communication_protocol_v1.0.pdf)

## Local Development

When editing this repository locally, point ESPHome at the local component:

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [ld2460]

uart:
  id: ld2460_bus
  tx_pin: GPIO16
  rx_pin: GPIO17
  baud_rate: 115200

ld2460:
  uart_id: ld2460_bus
```

## SmartHomeShop Tracking Package

For SmartHomeShop products, prefer `tracking-ld2460.yaml`. It exposes stable
internal IDs so firmware can swap LD2450 and LD2460 tracking packages without
rewriting occupancy logic:

- `tracking_presence`
- `tracking_target_count`
- `tracking_target_1_x`, `tracking_target_1_y`
- `tracking_target_1_distance`, `tracking_target_1_angle`
- same pattern for targets 2-5

UltimateSensor V2 schematic mapping:

```yaml
substitutions:
  ld2460_tx_pin: GPIO6 # ESP TX -> LD2460 RX
  ld2460_rx_pin: GPIO5 # ESP RX <- LD2460 TX

packages:
  tracking:
    url: https://github.com/smarthomeshop/ld2460
    ref: v0.1.0
    files:
      - tracking-ld2460.yaml
    refresh: 1d
```

UltimateSensor Mini V2 schematic mapping:

```yaml
substitutions:
  ld2460_tx_pin: GPIO4 # ESP TX -> LD2460 RX
  ld2460_rx_pin: GPIO5 # ESP RX <- LD2460 TX

packages:
  tracking:
    url: https://github.com/smarthomeshop/ld2460
    ref: v0.1.0
    files:
      - tracking-ld2460.yaml
    refresh: 1d
```

## Attribution

This repository is based on
[ciriousjoker/esphome_ld2460](https://github.com/ciriousjoker/esphome_ld2460)
by Philipp Bauer and remains available under the MIT License.
