// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/panel_window_event_filter.h"

#include "ash/wm/window_util.h"
#include "base/message_loop.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/cursor.h"
#include "ui/aura/event.h"
#include "ui/aura/root_window.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/base/hit_test.h"
#include "ui/base/ui_base_types.h"
#include "ui/gfx/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/screen.h"

namespace ash {
namespace internal {

PanelWindowEventFilter::PanelWindowEventFilter(
    aura::Window* panel_container,
    PanelLayoutManager* layout_manager)
    : aura::EventFilter(),
      panel_container_(panel_container),
      layout_manager_(layout_manager),
      dragged_panel_(NULL) {
}

PanelWindowEventFilter::~PanelWindowEventFilter() {
}

bool PanelWindowEventFilter::PreHandleKeyEvent(aura::Window* target,
                                               aura::KeyEvent* event) {
  return false;
}

bool PanelWindowEventFilter::PreHandleMouseEvent(aura::Window* target,
                                                 aura::MouseEvent* event) {
  switch (event->type()) {
    case ui::ET_MOUSE_PRESSED: {
      int hitResult = target->delegate()->
          GetNonClientComponent(event->location());
      if (hitResult == HTCAPTION) {
        StartDrag(target, event);
        return true;
      } else {
        return false;
      }
    }
    case ui::ET_MOUSE_DRAGGED:
      if (dragged_panel_ != NULL)
        return HandleDrag(target, event);
      return false;
    case ui::ET_MOUSE_RELEASED:
      if (dragged_panel_ != NULL) {
        CompleteDrag(target, event);
        return true;
      }
      return false;
    default:
      return false;
  }
}

ui::TouchStatus PanelWindowEventFilter::PreHandleTouchEvent(
    aura::Window* target,
    aura::TouchEvent* event) {
  return ui::TOUCH_STATUS_UNKNOWN;
}

ui::GestureStatus PanelWindowEventFilter::PreHandleGestureEvent(
    aura::Window* target, aura::GestureEvent* event) {
  return ui::GESTURE_STATUS_UNKNOWN;
}


void PanelWindowEventFilter::StartDrag(aura::Window* target,
                                       aura::LocatedEvent* event) {
  dragged_panel_ = target;
  drag_location_in_dragged_window_ = event->location();
  layout_manager_->StartDragging(target);
}

bool PanelWindowEventFilter::HandleDrag(aura::Window* target,
                                        aura::LocatedEvent* event) {
  gfx::Rect target_bounds = dragged_panel_->bounds();
  gfx::Point event_location_in_parent(event->location());
  aura::Window::ConvertPointToWindow(target,
                                     target->parent(),
                                     &event_location_in_parent);
  target_bounds.set_x(
      event_location_in_parent.x() - drag_location_in_dragged_window_.x());
  dragged_panel_->SetBounds(target_bounds);
  return true;

}

void PanelWindowEventFilter::CompleteDrag(aura::Window* target,
                                          aura::LocatedEvent* event) {
  dragged_panel_ = NULL;
  layout_manager_->FinishDragging();
}

}
}
