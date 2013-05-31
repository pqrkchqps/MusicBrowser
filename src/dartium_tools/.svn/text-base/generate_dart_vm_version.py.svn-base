import datetime
import imp
import subprocess
import sys
import time

utils = imp.load_source('utils', 'src/dart/tools/utils.py')


REVISION_FILE = 'src/chrome/browser/ui/webui/dartvm_revision.h'
EXPIRATION_FILE = 'src/third_party/WebKit/Source/bindings/dart/ExpirationTimeSecsSinceEpoch.time_t'


def write(filename, text):
  file(filename, 'w').write(text)

def equal(filename, text):
  return file(filename, 'r').read() == text

def main():
  # TODO: migrate to 2.7's subprocess.check_output once all are on 2.7.
  dart_version, _ = subprocess.Popen([utils.DartBinary(), 'tools/version.dart'],
      cwd='src/dart',
      stdout=subprocess.PIPE,
      shell=utils.IsWindows()).communicate()

  version_string = '#define DART_VM_REVISION "%s"\n' % dart_version.strip()

  if not equal(REVISION_FILE, version_string):
    write(REVISION_FILE, version_string)
    expiration_date = datetime.datetime.now() + datetime.timedelta(weeks=4)
    write(EXPIRATION_FILE, "%dLL\n" % time.mktime(expiration_date.timetuple()))

if __name__ == '__main__':
  main()
