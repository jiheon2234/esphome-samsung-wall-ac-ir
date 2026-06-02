# ESPHome Samsung Wall AC IR

![Samsung wall-mounted air conditioner](./photo/j2234-000395.png)

An ESPHome external component for controlling a Samsung wall-mounted air conditioner via IR.

This component is based on the IR packets captured from the Samsung `AR06M1130HZ` model.

## Supported Features

- Power OFF
- Cool mode
- Fan speed 1/2/3
- Temperature control from 16°C to 32°C

## References

- [Notes](notes.md)
- [Captured data](capture/)
- [Changelog](CHANGELOG.md)

## Usage

```yaml
...
external_components:
  - source:
      type: git
      url: https://github.com/jiheon2234/esphome-samsung-wall-ac-ir
      ref: v0.0.2
...
```
