#!/usr/bin/env python
#
# Copyright 2010 Google Inc. All Rights Reserved.

# This file is used by the buildbot.

import optparse
import utils

HOST_OS = utils.guessOS()
HOST_CPUS = utils.guessCpus()

TARGET_PROJECT = {
    'test_shell': 'webkit/webkit.xcodeproj',
    'DumpRenderTree': 'webkit/webkit.xcodeproj',
    'chrome': 'chrome/chrome.xcodeproj',
    'pkg_packages': 'dart/pkg/pkg.xcodeproj',
}

TARGET_RENAMES = {
    'DumpRenderTree': 'pull_in_DumpRenderTree',
}

ALL_TARGETS = TARGET_PROJECT.keys()


def main():
  parser = optparse.OptionParser()
  parser.add_option('--target', dest='target',
                    default='all',
                    action='store', type='string',
                    help='Target (%s)' % ', '.join(ALL_TARGETS))
  parser.add_option('--mode', dest='mode',
                    action='store', type='string',
                    help='Build mode (Debug or Release)')
  parser.add_option('--clobber', dest='clobber',
                    action='store_true',
                    help='Clobber the output directory')
  parser.add_option('-j', '--jobs', dest='jobs',
                    action='store',
                    help='Number of jobs')
  (options, args) = parser.parse_args()
  mode = options.mode
  if options.jobs:
    jobs = options.jobs
  else:
    jobs = HOST_CPUS
  if not (mode in ['Debug', 'Release']):
    raise Exception('Invalid build mode')

  if options.target == 'all':
    targets = ALL_TARGETS
  else:
    targets = [options.target]

  if HOST_OS == 'mac':
    if options.clobber:
      utils.runCommand(['rm', '-rf', 'xcodebuild'])
    for target in targets:
      project = TARGET_PROJECT[target]
      target = TARGET_RENAMES.get(target, target)
      utils.runCommand(['xcodebuild',
                      '-project', project,
                      '-parallelizeTargets',
                      '-configuration', mode,
                      '-target', target])
  elif HOST_OS == 'linux':
    if options.clobber:
      utils.runCommand(['rm', '-rf', 'out'])
    utils.runCommand(['make',
                      '-j%s' % jobs,
                      'BUILDTYPE=%s' % mode] + targets)

if __name__ == '__main__':
  main()
