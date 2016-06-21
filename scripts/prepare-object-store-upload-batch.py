#!/usr/bin/env python

# It would be much cleaner to do this with http PUT requests that specify "If-None-Match: <hash>"
# to make the upload conditional, but Swift currently only supports "If-None-Match: *", which
# is why we need to download and check the checksums ourselves.

import sys
import os
import hashlib
import csv

remote = {}

for name, hash in csv.reader(sys.stdin):
	remote[name] = hash

good = 0
bad = 0
seen = 0

dirlist = [x for x in os.listdir('.')
             if x.endswith(".gif") or x.endswith(".png")]

perpercent = len(dirlist) / 100

for f in dirlist:
	if seen % perpercent == 0:
		sys.stderr.write('\rComparing checksums: ' + str(seen / perpercent) + '%')
	seen += 1

	if f in remote and remote[f] == hashlib.md5(open(f, "rb").read()).hexdigest():
		good += 1
	else:
		bad += 1
		print f

sys.stderr.write('\nAlready there: ' + str(good))
sys.stderr.write(', to upload: ' + str(bad) + '\n')
