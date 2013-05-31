#!/usr/bin/env python
#
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Installs an APK.

"""

import optparse
import os
import subprocess
import sys

from util import build_utils
from util import md5_check

BUILD_ANDROID_DIR = os.path.join(os.path.dirname(__file__), '..')
sys.path.append(BUILD_ANDROID_DIR)

from pylib import android_commands

def main(argv):
  parser = optparse.OptionParser()
  parser.add_option('--android-sdk-tools',
      help='Path to Android SDK tools.')
  parser.add_option('--apk-path',
      help='Path to .apk to install.')
  parser.add_option('--stamp',
      help='Path to touch on success.')
  options, _ = parser.parse_args()

  # TODO(cjhopman): Should this install to all devices/be configurable?
  install_cmd = [
      os.path.join(options.android_sdk_tools, 'adb'),
      'install', '-r',
      options.apk_path]

  serial_number = android_commands.AndroidCommands().Adb().GetSerialNumber()
  md5_stamp = '%s.%s.md5' % (options.apk_path, serial_number)

  md5_checker = md5_check.Md5Checker(
      stamp=md5_stamp, inputs=[options.apk_path], command=install_cmd)
  if md5_checker.IsStale():
    build_utils.CheckCallDie(install_cmd)
    md5_checker.Write()

  if options.stamp:
    build_utils.Touch(options.stamp)


if __name__ == '__main__':
  sys.exit(main(sys.argv))
