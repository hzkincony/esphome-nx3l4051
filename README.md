# ESPHome NX3L4051 Component

ESPHome external component for controlling an NX3L4051 analog multiplexer.

It supports select lines driven by ESP GPIOs, or fixed hardware levels such as `LOW` / `HIGH`. This is useful for boards where one mux select pin is tied to GND or VCC.

## Use From Git

```yaml
external_components:
  - source:
      type: git
      url: git@github.com:hzkincony/esphome-nx3l4051.git
      ref: main
    components: [nx3l4051]
```

## Basic Configuration

```yaml
nx3l4051:
  - id: pt100_mux
    pin_s1: GPIO7
    pin_s2: GPIO21
    pin_s3: LOW
    delay: 20ms
```

`pin_s1` is the lowest select bit, followed by `pin_s2` and `pin_s3`.

Each select pin may be:

```yaml
pin_s1: GPIO25
pin_s2:
  number: GPIO26
  mode:
    output: true
pin_s3: LOW
```

`LOW` and `HIGH` mean the select line is fixed externally and is not controlled by ESPHome.

## Actions

Select a mux channel:

```yaml
- nx3l4051.select:
    id: pt100_mux
    channel: 0
```

Lambda channels are supported:

```yaml
- nx3l4051.select:
    id: pt100_mux
    channel: !lambda "return id(current_channel);"
```

Disable the mux if `enable_pin` is configured:

```yaml
- nx3l4051.disable:
    id: pt100_mux
```

`enable_pin` is optional. If omitted, `disable` is a no-op.

## PT100 / MAX31865 Example

For 4-channel 3-wire PT100 hardware, use two NX3L4051 chips with shared select pins: one mux switches the `A` lines, the other switches the `B` lines. The third PT100 wire is common. MAX31865 performs the RTD measurement.

See [examples/pt100_4ch_max31865.yaml](examples/pt100_4ch_max31865.yaml).
