// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file provides a parser for PReg files which are used for storing group
// policy settings in the file system. The file format is documented here:
//
// http://msdn.microsoft.com/en-us/library/windows/desktop/aa374407(v=vs.85).aspx

#ifndef CHROME_BROWSER_POLICY_PREG_PARSER_WIN_H_
#define CHROME_BROWSER_POLICY_PREG_PARSER_WIN_H_

#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/string16.h"

namespace base {
class DictionaryValue;
class FilePath;
}

namespace policy {
namespace preg_parser {

// Reads the PReg file at |file_path| and writes the registry data to |dict|.
// |root| specifies the registry subtree the caller is interested in,
// everything else gets ignored.
bool ReadFile(const base::FilePath& file_path,
              const string16& root,
              base::DictionaryValue* dict);

}  // namespace preg_parser
}  // namespace policy

#endif  // CHROME_BROWSER_POLICY_PREG_PARSER_WIN_H_
