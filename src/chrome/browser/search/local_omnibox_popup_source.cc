// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search/local_omnibox_popup_source.h"

#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/string_util.h"
#include "base/stringprintf.h"
#include "chrome/common/url_constants.h"
#include "googleurl/src/gurl.h"
#include "grit/browser_resources.h"
#include "net/url_request/url_request.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

const char kHtmlFilename[] = "local-omnibox-popup.html";
const char kJSFilename[] = "local-omnibox-popup.js";
const char kCssFilename[] = "local-omnibox-popup.css";
const char kPageIconFilename[] = "images/page_icon.png";
const char kPageIcon2xFilename[] = "images/2x/page_icon.png";
const char kSearchIconFilename[] = "images/search_icon.png";
const char kSearchIcon2xFilename[] = "images/2x/search_icon.png";

}  // namespace

LocalOmniboxPopupSource::LocalOmniboxPopupSource() {
}

LocalOmniboxPopupSource::~LocalOmniboxPopupSource() {
}

std::string LocalOmniboxPopupSource::GetSource() {
  return chrome::kChromeSearchLocalOmniboxPopupHost;
}

void LocalOmniboxPopupSource::StartDataRequest(
    const std::string& path,
    bool is_incognito,
    const content::URLDataSource::GotDataCallback& callback) {
  int identifier = -1;
  if (path == kHtmlFilename) {
    identifier = IDR_LOCAL_OMNIBOX_POPUP_HTML;
  } else if (path == kJSFilename) {
    identifier = IDR_LOCAL_OMNIBOX_POPUP_JS;
  } else if (path == kCssFilename) {
    identifier = IDR_LOCAL_OMNIBOX_POPUP_CSS;
  } else if (path == kPageIconFilename) {
    identifier = IDR_LOCAL_OMNIBOX_POPUP_IMAGES_PAGE_ICON_PNG;
  } else if (path == kPageIcon2xFilename) {
    identifier = IDR_LOCAL_OMNIBOX_POPUP_IMAGES_2X_PAGE_ICON_PNG;
  } else if (path == kSearchIconFilename) {
    identifier = IDR_LOCAL_OMNIBOX_POPUP_IMAGES_SEARCH_ICON_PNG;
  } else if (path == kSearchIcon2xFilename) {
    identifier = IDR_LOCAL_OMNIBOX_POPUP_IMAGES_2X_SEARCH_ICON_PNG;
  } else {
    callback.Run(NULL);
    return;
  }

  scoped_refptr<base::RefCountedStaticMemory> response(
      ResourceBundle::GetSharedInstance().LoadDataResourceBytes(identifier));
  callback.Run(response);
}

std::string LocalOmniboxPopupSource::GetMimeType(
    const std::string& path) const {
  if (path == kHtmlFilename)
    return "text/html";
  if (path == kJSFilename)
    return "application/javascript";
  if (path == kCssFilename)
    return "text/css";
  if (path == kPageIconFilename || path == kPageIcon2xFilename ||
      path == kSearchIconFilename || path == kSearchIcon2xFilename)
    return "image/png";
  return std::string();
}

bool LocalOmniboxPopupSource::ShouldServiceRequest(
    const net::URLRequest* request) const {
  DCHECK(request->url().host() == chrome::kChromeSearchLocalOmniboxPopupHost);

  if (request->url().SchemeIs(chrome::kChromeSearchScheme)) {
    DCHECK(StartsWithASCII(request->url().path(), "/", true));
    std::string filename = request->url().path().substr(1);
    return filename == kHtmlFilename || filename == kJSFilename ||
           filename == kCssFilename || filename == kPageIconFilename ||
           filename == kPageIcon2xFilename || filename == kSearchIconFilename ||
           filename == kSearchIcon2xFilename;
  }
  return false;
}

std::string LocalOmniboxPopupSource::GetContentSecurityPolicyFrameSrc() const {
  // Allow embedding of chrome search suggestion host.
  return base::StringPrintf("frame-src %s://%s/;",
                            chrome::kChromeSearchScheme,
                            chrome::kChromeSearchSuggestionHost);
}
