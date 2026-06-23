# Phase 10 Default Certificate File Creation

This package creates demo certificate files in the root of the Quectel `U:` filesystem when they are missing.

## Files created

```text
U:/mqtt_root_ca.pem
U:/mqtt_client.crt
U:/mqtt_client.key
U:/http_root_ca.pem
```

## Source of defaults

The default PEM strings are migrated from the supplied EC200U-CN-AA application source, TEST_SERVER certificate buffer section.

## Runtime behavior

- Existing files are never overwritten.
- Missing files are created during application boot after the `U:` filesystem/config service is initialized.
- The MQTT and HTTP services then use the existing root-level paths.
- HTTP root CA is created using the same default root CA PEM so the previous missing-file guard does not block the demo.

## Security note

These defaults are for lab/demo migration compatibility. For production, provision unique per-device certificates/keys and do not share one private key across production units.
