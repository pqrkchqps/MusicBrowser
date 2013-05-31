#!/usr/bin/env python
#
# Copyright 2012 Google Inc. All Rights Reserved.

# List of files that are overriden.

def makeFilePath(path):
  return ('src/dartium_tools/overrides/' + path, 'src/' + path)

OVERRIDDEN_FILES = [
  # User-agent and chrome://version.
  makeFilePath('chrome/common/chrome_content_client.cc'),
  makeFilePath('chrome/browser/resources/about_version.html'),
  makeFilePath('chrome/browser/ui/webui/version_ui.cc'),
  makeFilePath('chrome/tools/build/mac/verify_order'),

  # Temporary hack for dartium_builder.
  makeFilePath('build/all.gyp'),
  # Temporary hack ro fix Mac build.
  makeFilePath('build/common.gypi'),
]
