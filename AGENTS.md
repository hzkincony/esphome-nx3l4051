# Repository Guidelines

## Project Structure & Module Organization

This repository contains a local ESPHome external component for the NX3L4051 analog mux.

- `components/nx3l4051/`: component implementation.
  - `__init__.py`: ESPHome config schema, code generation, and automation action registration.
  - `nx3l4051.h` / `nx3l4051.cpp`: runtime GPIO control and action classes.
- `examples/`: ESPHome YAML examples. `pt100_4ch_max31865.yaml` demonstrates 4-channel PT100 measurement with MAX31865.
- `.nix/`: Nix flake for a reproducible ESPHome development shell.

There is no separate test suite yet; validation is done with ESPHome config and compile checks.

## Build, Test, and Development Commands

Use the Nix shell for all ESPHome work:

```bash
nix develop ./.nix
```

Validate the example YAML:

```bash
nix develop ./.nix -c esphome config examples/pt100_4ch_max31865.yaml
```

Compile the example firmware:

```bash
nix develop ./.nix -c esphome compile examples/pt100_4ch_max31865.yaml
```

Run Python syntax checks for codegen files:

```bash
python3 -m py_compile components/nx3l4051/__init__.py
```

## Coding Style & Naming Conventions

Python files use 4-space indentation and ESPHome conventions: `CONF_*` constants for YAML keys, `CONFIG_SCHEMA` for validation, and `to_code()` for code generation. C++ follows ESPHome style: two-space indentation, `snake_case` methods, private members with trailing underscores, and `ESP_LOG*` macros for diagnostics.

Keep component IDs, action names, and YAML keys stable. Public action names are `nx3l4051.select` and `nx3l4051.disable`.

## Testing Guidelines

At minimum, run `esphome config` after schema or YAML changes. Run `esphome compile` after C++ changes or action signature changes. Test both GPIO shorthand and fixed-level select pins, for example:

```yaml
pin_s1: GPIO7
pin_s2: GPIO21
pin_s3: LOW
```

## Commit & Pull Request Guidelines

Use Conventional Commit messages, matching current history:

```text
feat(nx3l4051): add ESPHome mux component
```

PRs should include a short summary, relevant YAML examples, validation commands run, and hardware notes if behavior depends on board wiring. Do not commit generated ESPHome build output such as `.esphome/`, `.pioenvs/`, or `.piolibdeps/`.

## Security & Configuration Tips

Do not commit real Wi-Fi credentials or `secrets.yaml`. Example YAML should use placeholders or ESPHome secrets for sensitive values.
