// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_UI_CAPTIVE_PORTAL_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_UI_CAPTIVE_PORTAL_VIEW_H_

#include <string>
#include "chrome/browser/chromeos/login/ui/simple_web_view_dialog.h"

namespace chromeos {

class CaptivePortalWindowProxy;

class CaptivePortalView : public SimpleWebViewDialog {
 public:
  CaptivePortalView(Profile* profile, CaptivePortalWindowProxy* proxy);
  virtual ~CaptivePortalView();

  // Starts loading.
  void StartLoad();

  // Overridden from views::WidgetDelegate:
  virtual bool CanResize() const OVERRIDE;
  virtual ui::ModalType GetModalType() const OVERRIDE;
  virtual base::string16 GetWindowTitle() const OVERRIDE;
  virtual bool ShouldShowWindowTitle() const OVERRIDE;
  virtual views::NonClientFrameView* CreateNonClientFrameView(
      views::Widget* widget) OVERRIDE;

  // Overridden from content::WebContentsDelegate:
  virtual void NavigationStateChanged(
      const content::WebContents* source,
      content::InvalidateTypes changed_flags) OVERRIDE;
  virtual void LoadingStateChanged(content::WebContents* source,
                                   bool to_different_document) OVERRIDE;

 private:
  // Contains CaptivePortalWindowProxy to be notified when redirection state is
  // resolved.
  CaptivePortalWindowProxy* proxy_;

  bool redirected_;

  DISALLOW_COPY_AND_ASSIGN(CaptivePortalView);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_UI_CAPTIVE_PORTAL_VIEW_H_
