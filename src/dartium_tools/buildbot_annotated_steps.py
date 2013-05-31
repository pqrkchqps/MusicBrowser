#!/usr/bin/python

# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Chromium buildbot steps

Run the Dart layout tests.
"""

import glob
import os
import platform
import re
import shutil
import socket
import subprocess
import sys
import imp

BUILDER_NAME = 'BUILDBOT_BUILDERNAME'
REVISION = 'BUILDBOT_REVISION'
BUILDER_PATTERN = r'dartium-(\w+)-(\w+)'

if platform.system() == 'Windows':
  GSUTIL = 'e:/b/build/scripts/slave/gsutil.bat'
else:
  GSUTIL = '/b/build/scripts/slave/gsutil'
ACL = 'public-read'
GS_SITE = 'gs://'
GS_URL = 'https://sandbox.google.com/storage/'
GS_DIR = 'dartium-archive'
GS_LAYOUT_TEST_RESULTS_DIR = os.path.join(GS_DIR, 'layout-test-results')
LATEST = 'latest'
CONTINUOUS = 'continuous'

REVISION_FILE = 'chrome/browser/ui/webui/dartvm_revision.h'

# Add dartium tools and build/util to python path.
SRC_PATH = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS_PATH = os.path.join(SRC_PATH, 'dartium_tools')
DART_PATH = os.path.join(SRC_PATH, 'dart')
BUILD_UTIL_PATH = os.path.join(SRC_PATH, 'build/util')
sys.path.extend([TOOLS_PATH, BUILD_UTIL_PATH])
import archive
import utils

def ExecuteCommand(cmd):
  """Execute a command in a subprocess.
  """
  print 'Executing: ' + ' '.join(cmd)
  try:
    pipe = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (output, error) = pipe.communicate()
    if pipe.returncode != 0:
      print 'Execution failed: ' + str(error)
    return (pipe.returncode, output)
  except:
    import traceback
    print 'Execution raised exception:', traceback.format_exc()
    return (-1, '')


def GetBuildInfo():
  """Returns a tuple (name, version, mode, do_archive) where:
    - name: A name for the build - the buildbot host if a buildbot.
    - version: A version string corresponding to this build.
    - mode: 'Debug' or 'Release'
    - arch: target architecture
    - do_archive: True iff an archive should be generated and uploaded.
    - is_trunk: True if this is a trunk build.
  """
  os.chdir(SRC_PATH)

  name = None
  version = None
  mode = 'Release'
  do_archive = False

  # Populate via builder environment variables.
  name = os.environ.get(BUILDER_NAME)

  # We need to chdir() to src/dart in order to get the correct revision number.
  with utils.ChangedWorkingDirectory(DART_PATH):
    dart_tools_utils = imp.load_source('dart_tools_utils',
                                       os.path.join('tools', 'utils.py'))
    dart_version = dart_tools_utils.GetVersion()
    match = re.search('._r(\d+)', dart_version)
    dart_revision = match.group(1)

  version = dart_revision + '.0'
  is_trunk = 'trunk' in name

  if name:
    pattern = re.match(BUILDER_PATTERN, name)
    if pattern:
      arch = 'x64' if pattern.group(1) == 'lucid64' else 'ia32'
      if pattern.group(2) == 'debug':
        mode = 'Debug'
      do_archive = True

  # Fall back if not on builder.
  if not name:
    name = socket.gethostname().split('.')[0]

  return (name, version, mode, arch, do_archive, is_trunk)


def RunDartTests(mode, component, suite, arch, checked):
  """Runs the Dart WebKit Layout tests.
  """
  cmd = [sys.executable]
  script = os.path.join(TOOLS_PATH, 'test.py')
  cmd.append(script)
  cmd.append('--buildbot')
  cmd.append('--mode=' + mode)
  cmd.append('--component=' + component)
  cmd.append('--suite=' + suite)
  cmd.append('--arch=' + arch)
  cmd.append('--' + checked)
  cmd.append('--no-show-results')
  status = subprocess.call(cmd)
  if status != 0:
    print '@@@STEP_FAILURE@@@'
  return status


def UploadDartTestsResults(layout_test_results_dir, name, version,
                           component, checked):
  """Uploads test results to google storage.
  """
  print ('@@@BUILD_STEP archive %s_layout_%s_tests results@@@' %
         (component, checked))
  dir_name = os.path.dirname(layout_test_results_dir)
  base_name = os.path.basename(layout_test_results_dir)
  cwd = os.getcwd()
  os.chdir(dir_name)

  archive_name = 'layout_test_results.zip'
  archive.ZipDir(archive_name, base_name)

  target = '/'.join([GS_LAYOUT_TEST_RESULTS_DIR, name, component + '-' +
                     checked + '-' + version + '.zip'])
  status = UploadArchive(os.path.abspath(archive_name), GS_SITE + target)
  os.remove(archive_name)
  if status == 0:
    print ('@@@STEP_LINK@download@' + GS_URL + target + '@@@')
  else:
    print '@@@STEP_FAILURE@@@'
  os.chdir(cwd)


def ListArchives(pattern):
  """List the contents in Google storage matching the file pattern.
  """
  cmd = [GSUTIL, 'ls', pattern]
  (status, output) = ExecuteCommand(cmd)
  if status != 0:
    return []
  return output.split(os.linesep)


def RemoveArchives(archives):
  """Remove the list of archives in Google storage.
  """
  for archive in archives:
    if archive.find(GS_SITE) == 0:
      cmd = [GSUTIL, 'rm', archive.rstrip()]
      (status, _) = ExecuteCommand(cmd)
      if status != 0:
        return status
  return 0


def UploadArchive(source, target):
  """Upload an archive zip file to Google storage.
  """
  # See http://code.google.com/p/gsutil/issues/detail?id=76 .
  source = os.path.splitdrive(source)[1]

  # Upload file.
  cmd = [GSUTIL, 'cp', source, target]
  (status, output) = ExecuteCommand(cmd)
  if status != 0:
    return status
  print 'Uploaded: ' + output

  # Set ACL.
  if ACL is not None:
    cmd = [GSUTIL, 'setacl', ACL, target]
    (status, output) = ExecuteCommand(cmd)
  return status


def main():
  (dartium_bucket, version, mode, arch, do_upload, is_trunk) = GetBuildInfo()
  drt_bucket = dartium_bucket.replace('dartium', 'drt')
  chromedriver_bucket = dartium_bucket.replace('dartium', 'chromedriver')

  def archiveAndUpload():
    print '@@@BUILD_STEP dartium_generate_archive@@@'
    cwd = os.getcwd()
    dartium_archive = dartium_bucket + '-' + version
    drt_archive = drt_bucket + '-' + version
    chromedriver_archive = chromedriver_bucket + '-' + version
    dartium_zip, drt_zip, chromedriver_zip = \
        archive.Archive(SRC_PATH, mode, dartium_archive,
                        drt_archive, chromedriver_archive)
    status = upload('dartium', dartium_bucket, os.path.abspath(dartium_zip))
    if status == 0:
      status = upload('drt', drt_bucket, os.path.abspath(drt_zip))
    if status == 0:
      status = upload('chromedriver', chromedriver_bucket,
                      os.path.abspath(chromedriver_zip))
    os.chdir(cwd)
    if status != 0:
      print '@@@STEP_FAILURE@@@'
    return status

  def upload(module, bucket, zip_file):
    print '@@@BUILD_STEP %s_upload_archive@@@' % module
    path, filename = os.path.split(zip_file)
    target = '/'.join([GS_DIR, bucket, filename])
    status = UploadArchive(zip_file, GS_SITE + target)
    print ('@@@STEP_LINK@download@' + GS_URL + target + '@@@')

    print '@@@BUILD_STEP %s_upload_latest@@@' % module
    # Clear latest for this build type.
    old = '/'.join([GS_DIR, LATEST, bucket + '-*'])
    old_archives = ListArchives(GS_SITE + old)

    # Upload the new latest and remove unnecessary old ones.
    target = GS_SITE + '/'.join([GS_DIR, LATEST, filename])
    status = UploadArchive(zip_file, target)
    if status == 0:
      RemoveArchives(
          [arch for arch in old_archives if arch != target])
    else:
      print 'Upload failed'

    # Upload unversioned name to continuous site for incremental
    # builds.
    if bucket.endswith('-inc'):
      continuous_name = bucket.replace('-inc', '')
      target = GS_SITE + '/'.join([GS_DIR, CONTINUOUS, continuous_name + '.zip'])
      status = UploadArchive(zip_file, target)

    print '@@@BUILD_STEP %s_upload_archive is over (status = %s)@@@' % (module, status)

    return status

  def test(component, suite, checked):
    """Test a particular component (e.g., dartium or frog).
    """
    print '@@@BUILD_STEP %s_%s_%s_tests@@@' % (component, suite, checked)
    sys.stdout.flush()
    layout_test_results_dir = os.path.join(SRC_PATH, 'webkit', mode,
                                           'layout-test-results')
    shutil.rmtree(layout_test_results_dir, ignore_errors=True)
    status = RunDartTests(mode, component, suite, arch, checked)

    if suite == 'layout' and status != 0:
      UploadDartTestsResults(layout_test_results_dir, dartium_bucket, version,
                             component, checked)
    return status

  result = 0

  # if we are on trunk we archive first
  if is_trunk:
    result = archiveAndUpload()

  # Run Dartium tests
  if mode == 'Release' or platform.system() != 'Darwin':
    result = test('drt', 'layout', 'unchecked') or result
    result = test('drt', 'layout', 'checked') or result
  if mode == 'Release':
    # Don't run these by default in Debug.  They take very long.
    result = test('drt', 'core', 'unchecked') or result
    result = test('drt', 'core', 'checked') or result
    # TODO(antonm): temporary disable it.
    # result = test('dartium', 'core', 'unchecked') or result

  # If Dartium tests pass, upload binary archive, on trunk we already archived
  if result == 0 and do_upload and not is_trunk:
    result = archiveAndUpload()

if __name__ == '__main__':
  sys.exit(main())
