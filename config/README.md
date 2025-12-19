# blah2 Configuration

blah2-arm uses a layered configuration system. Default configs are baked into containers, and user settings persist between updates. New configurations options can be easily introduced, updated and deployed to existing nodes by updating `default.yml`. Note this configuration management is currently only supported for RSPDuo,

## Configuration Files

- **default.yml** - Default config settings (baked into containers)
- **forced.yml** - Forced config overrides, should they be required (baked into containers)
- **user.yml** - User customisations (persists in `/data/blah2/config/`)

## How It Works

On first run, `user.yml` is automatically created in `/data/blah2/config/user.yml`.

Configs are merged in this order (later overrides earlier):
1. **default.yml**
2. **user.yml**
3. **forced.yml**

Both `blah2` and `blah2-api` containers independently merge configs on startup to avoid race conditions. Read-only debug outputs showing the full merged config are written to:
- `/data/blah2/config/config.blah2.debug.yml`
- `/data/blah2/config/config.api.debug.yml`

## Customising a Node

1. Edit `/data/blah2/config/user.yml`
2. Restart blah2 and blah2-api containers: `sudo docker restart blah2 blah2-api blah2-web blah2-host`
3. Verify changes: `cat /data/blah2/config/config.blah2.debug.yml` and `cat /data/blah2/config/config.api.debug.yml`, these files should be identical which can be verified with `diff /data/blah2/config/config.blah2.debug.yml /data/blah2/config/config.api.debug.yml` the output should show no differences.

