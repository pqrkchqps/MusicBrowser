#!/usr/bin/env python
#
# Copyright 2012 Google Inc. All Rights Reserved.

import overrides_database
import shutil
import subprocess
import utils
import filecmp


if __name__ == '__main__':
  for patched, orig in overrides_database.OVERRIDDEN_FILES:
    if not filecmp.cmp(patched, orig):
      shutil.copyfile(patched, orig)
