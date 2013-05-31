#!/usr/bin/python

# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob
import optparse
import os
import shutil
import subprocess
import sys
import utils

HOST_OS = utils.guessOS()

if HOST_OS == 'mac':
  VERSION_FILE = 'Chromium.app/Contents/MacOS/VERSION'
  DRT_FILES = ['DumpRenderTree.app', 'ffmpegsumo.so', 'osmesa.so']
  CHROMEDRIVER_FILES = ['chromedriver']
elif HOST_OS == 'linux':
  VERSION_FILE = 'VERSION'
  DRT_FILES = ['DumpRenderTree', 'DumpRenderTree.pak', 'fonts.conf',
               'libffmpegsumo.so', 'libosmesa.so']
  CHROMEDRIVER_FILES = ['chromedriver']
elif HOST_OS == 'win':
  VERSION_FILE = 'VERSION'
  # TODO: provide proper list.
  DRT_FILES = ['DumpRenderTree.exe', 'AHEM____.ttf']
  CHROMEDRIVER_FILES = ['chromedriver.exe']
else:
  raise Exception('Unsupported platform')

# Append a file with size of the snapshot.
DRT_FILES.append('snapshot-size.txt')


def GenerateVersionFile():
  # TODO: fix it.
  if HOST_OS == 'win': return
  versionInfo = utils.getCommandOutput(os.path.join('..', '..',
                                                    'dartium_tools',
                                                    'print_dart_version.sh'))
  file = open(VERSION_FILE, 'w')
  file.write(versionInfo)
  file.close()


def GenerateDartiumFileList(srcpath):
  configFile = os.path.join(srcpath, 'chrome', 'tools', 'build', HOST_OS,
                            'FILES.cfg')
  configNamespace = {}
  execfile(configFile, configNamespace)
  fileList = [file['filename'] for file in configNamespace['FILES']]
  # Remove this once downstream test bots use the DRT zip.
  fileList.extend(DRT_FILES)
  return fileList


def GenerateDRTFileList(srcpath):
  return DRT_FILES


def GenerateChromeDriverFileList(srcpath):
  return CHROMEDRIVER_FILES


def ZipDir(zipFile, directory):
  if HOST_OS == 'win':
    cmd = os.path.normpath(os.path.join(
        os.path.dirname(__file__),
        '../third_party/lzma_sdk/Executable/7za.exe'))
    options = ['a', '-r', '-tzip']
  else:
    cmd = 'zip'
    options = ['-yr']
  utils.runCommand([cmd] + options + [zipFile, directory])


def GenerateZipFile(zipFile, stageDir, fileList):
  # Stage files.
  for fileName in fileList:
    fileName = fileName.rstrip(os.linesep)
    targetName = os.path.join(stageDir, fileName)
    try:
      targetDir = os.path.dirname(targetName)
      if not os.path.exists(targetDir):
        os.makedirs(targetDir)
      if os.path.isdir(fileName):
        shutil.copytree(fileName, targetName)
      elif os.path.exists(fileName):
        shutil.copy2(fileName, targetName)
    except:
      import traceback
      print 'Troubles processing %s [cwd=%s]: %s' % (fileName, os.getcwd(), traceback.format_exc())

  ZipDir(zipFile, stageDir)


def StageAndZip(fileList, target):
  if not target:
    return None

  stageDir = target
  zipFile = stageDir + '.zip'

  # Cleanup old files.
  if os.path.exists(stageDir):
    shutil.rmtree(stageDir)
  os.mkdir(stageDir)
  oldFiles = glob.glob(target.split('-')[0] + '*.zip')
  for oldFile in oldFiles:
    os.remove(oldFile)

  GenerateVersionFile()
  GenerateZipFile(zipFile, stageDir, fileList)
  print 'last change: %s' % (zipFile)

  # Clean up. Buildbot disk space is limited.
  shutil.rmtree(stageDir)

  return zipFile


def Archive(srcpath, mode, dartium_target, drt_target, chromedriver_target):
  # We currently build using ninja on mac debug.
  if HOST_OS == 'mac':
    releaseDir = os.path.join(srcpath, 'out', mode)
  elif HOST_OS == 'linux':
    releaseDir = os.path.join(srcpath, 'out', mode)
  elif HOST_OS == 'win':
    releaseDir = os.path.join(srcpath, 'build', mode)
  else:
    raise Exception('Unsupported platform')
  os.chdir(releaseDir)

  dartium_zip = StageAndZip(GenerateDartiumFileList(srcpath), dartium_target)
  drt_zip = StageAndZip(GenerateDRTFileList(srcpath), drt_target)
  chromedriver_zip = StageAndZip(GenerateChromeDriverFileList(srcpath),
                                 chromedriver_target)
  return (dartium_zip, drt_zip, chromedriver_zip)


def main():
  pathname = os.path.dirname(sys.argv[0])
  fullpath = os.path.abspath(pathname)
  srcpath = os.path.join(fullpath, '..')

  parser = optparse.OptionParser()
  parser.add_option('--dartium', dest='dartium',
                    action='store', type='string',
                    help='dartium archive name')
  parser.add_option('--drt', dest='drt',
                    action='store', type='string',
                    help='DumpRenderTree archive name')
  parser.add_option('--chromedriver', dest='chromedriver',
                    action='store', type='string',
                    help='chromedriver archive name')
  (options, args) = parser.parse_args()
  Archive(srcpath, 'Release', options.dartium, options.drt, options.chromedriver)
  return 0


if __name__ == '__main__':
  sys.exit(main())
