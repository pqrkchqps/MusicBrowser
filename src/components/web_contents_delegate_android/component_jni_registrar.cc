// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/web_contents_delegate_android/component_jni_registrar.h"

#include "base/android/jni_android.h"
#include "base/android/jni_registrar.h"
#include "components/web_contents_delegate_android/color_chooser_android.h"
#include "components/web_contents_delegate_android/web_contents_delegate_android.h"

namespace components {

static base::android::RegistrationMethod kComponentRegisteredMethods[] = {
  { "ColorChooserAndroid", RegisterColorChooserAndroid },
  { "WebContentsDelegateAndroid", RegisterWebContentsDelegateAndroid },
};

bool RegisterWebContentsDelegateAndroidJni(JNIEnv* env) {
  return RegisterNativeMethods(env,
      kComponentRegisteredMethods, arraysize(kComponentRegisteredMethods));
}

} // namespace components
