#!/bin/bash
set -ev

# This script will ask for the password twice, once for openstack and once for sftp.

# TODO: Only ask once.

export OS_USERNAME="eelis"
export OS_PROJECT_ID="b295c08d343b4cf78a66f072d529c63c"
export OS_AUTH_URL="https://identity.stack.cloudvps.com/v2.0"
ftpserver="ftp.objectstore.eu"
container="gm" # TODO: this is also hard-coded in the python script. pass it instead

openstack object list --all --long --format csv -c Name -c Hash --quote none "${container}" |
  tail -n +2 |
  python scripts/prepare-object-store-upload-batch.py > batch

# The openstack CLI does marker chaining for us (which is why we can't just use curl).

sftp -q -o "User=${OS_PROJECT_ID}:${OS_USERNAME}" "${ftpserver}" < batch
