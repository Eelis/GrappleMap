#!/bin/bash

# Run this in the GrappleMap/images dir.

set -e

export ST_AUTH_VERSION=2
export OS_USERNAME="eelis"
export OS_PASSWORD="******";
export OS_PROJECT_ID="b295c08d343b4cf78a66f072d529c63c"
export OS_TENANT_ID="b295c08d343b4cf78a66f072d529c63c"
export OS_AUTH_URL="https://identity.stack.cloudvps.com/v2.0"
container="gm"

openstack object list --all --long --format csv -c Name -c Hash --quote none "${container}" |
  tail -n +2 |
  python ../../scripts/prepare-object-store-upload-batch.py > batch

echo "Now run: cat batch | xargs -n64 swift upload ${container}"
