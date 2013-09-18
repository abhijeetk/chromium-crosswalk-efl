// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_view.h"

#include "chrome/browser/bookmarks/bookmark_pasteboard_helper_mac.h"
#include "chrome/browser/profiles/profile.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_folder_target.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "content/public/browser/user_metrics.h"

using content::UserMetricsAction;

#import "third_party/mozilla/NSPasteboard+Utils.h"

@interface BookmarkBarFolderView()

@property(readonly, nonatomic) id<BookmarkButtonControllerProtocol> controller;

- (void)setDropIndicatorShown:(BOOL)flag;

@end

@implementation BookmarkBarFolderView

- (void)awakeFromNib {
  NSArray* types = [NSArray arrayWithObjects:
                    NSStringPboardType,
                    NSHTMLPboardType,
                    NSURLPboardType,
                    kBookmarkButtonDragType,
                    kBookmarkDictionaryListPboardType,
                    nil];
  [self registerForDraggedTypes:types];
}

- (void)dealloc {
  [self unregisterDraggedTypes];
  [super dealloc];
}

- (id<BookmarkButtonControllerProtocol>)controller {
  // When needed for testing, set the local data member |controller_| to
  // the test controller.
  return controller_ ? controller_ : [[self window] windowController];
}

// TODO(mrossetti,jrg): Identical to -[BookmarkBarView
// dragClipboardContainsBookmarks].  http://crbug.com/35966
// Shim function to assist in unit testing.
- (BOOL)dragClipboardContainsBookmarks {
  return bookmark_pasteboard_helper_mac::PasteboardContainsBookmarks(
      bookmark_pasteboard_helper_mac::kDragPasteboard);
}

// Virtually identical to [BookmarkBarView draggingEntered:].
// TODO(jrg): find a way to share code.  Lack of multiple inheritance
// makes things more of a pain but there should be no excuse for laziness.
// http://crbug.com/35966
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)info {
  inDrag_ = YES;
  if (![[self controller] draggingAllowed:info])
    return NSDragOperationNone;
  if ([[info draggingPasteboard] dataForType:kBookmarkButtonDragType] ||
      [self dragClipboardContainsBookmarks] ||
      [[info draggingPasteboard] containsURLData]) {
    // Find the position of the drop indicator.
    BOOL showIt = [[self controller]
                   shouldShowIndicatorShownForPoint:[info draggingLocation]];
    if (!showIt) {
      [self setDropIndicatorShown:NO];
    } else {
      [self setDropIndicatorShown:YES];

      CGFloat y = [[self controller]
          indicatorPosForDragToPoint:[info draggingLocation]];
      NSRect frame = [dropIndicator_ frame];
      if (NSMinY(frame) != y) {
        frame.origin.y = y;
        [dropIndicator_ setFrame:frame];
      }
    }

    [[self controller] draggingEntered:info];  // allow hover-open to work
    return [info draggingSource] ? NSDragOperationMove : NSDragOperationCopy;
  }
  return NSDragOperationNone;
}

- (void)draggingExited:(id<NSDraggingInfo>)info {
  [[self controller] draggingExited:info];

  // Regardless of the type of dragging which ended, we need to get rid of the
  // drop indicator if one was shown.
  [self setDropIndicatorShown:NO];
}

- (void)draggingEnded:(id<NSDraggingInfo>)info {
  // Awkwardness since views open and close out from under us.
  if (inDrag_) {
    inDrag_ = NO;
  }

  [self draggingExited:info];
}

- (BOOL)wantsPeriodicDraggingUpdates {
  // TODO(jrg): This should probably return |YES| and the controller should
  // slide the existing bookmark buttons interactively to the side to make
  // room for the about-to-be-dropped bookmark.
  // http://crbug.com/35968
  return NO;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)info {
  // For now it's the same as draggingEntered:.
  // TODO(jrg): once we return YES for wantsPeriodicDraggingUpdates,
  // this should ping the [self controller] to perform animations.
  // http://crbug.com/35968
  return [self draggingEntered:info];
}

- (BOOL)prepareForDragOperation:(id<NSDraggingInfo>)info {
  return YES;
}

// This code is practically identical to the same function in BookmarkBarView
// with the only difference being how the controller is retrieved.
// TODO(mrossetti,jrg): http://crbug.com/35966
// Implement NSDraggingDestination protocol method
// performDragOperation: for URLs.
- (BOOL)performDragOperationForURL:(id<NSDraggingInfo>)info {
  NSPasteboard* pboard = [info draggingPasteboard];
  DCHECK([pboard containsURLData]);

  NSArray* urls = nil;
  NSArray* titles = nil;
  [pboard getURLs:&urls andTitles:&titles convertingFilenames:YES];

  return [[self controller] addURLs:urls
                         withTitles:titles
                                 at:[info draggingLocation]];
}

// This code is practically identical to the same function in BookmarkBarView
// with the only difference being how the controller is retrieved.
// http://crbug.com/35966
// Implement NSDraggingDestination protocol method
// performDragOperation: for bookmark buttons.
- (BOOL)performDragOperationForBookmarkButton:(id<NSDraggingInfo>)info {
  BOOL doDrag = NO;
  NSData* data = [[info draggingPasteboard]
                   dataForType:kBookmarkButtonDragType];
  // [info draggingSource] is nil if not the same application.
  if (data && [info draggingSource]) {
    BookmarkButton* button = nil;
    [data getBytes:&button length:sizeof(button)];

    // If we're dragging from one profile to another, disallow moving (only
    // allow copying). Each profile has its own bookmark model, so one way to
    // check whether we are dragging across profiles is to see if the
    // |BookmarkNode| corresponding to |button| exists in this profile. If it
    // does, we're dragging within a profile; otherwise, we're dragging across
    // profiles.
    const BookmarkModel* const model = [[self controller] bookmarkModel];
    const BookmarkNode* const source_node = [button bookmarkNode];
    const BookmarkNode* const target_node =
        model->GetNodeByID(source_node->id());

    BOOL copy =
        !([info draggingSourceOperationMask] & NSDragOperationMove) ||
        (source_node != target_node);
    doDrag = [[self controller] dragButton:button
                                        to:[info draggingLocation]
                                      copy:copy];
    content::RecordAction(UserMetricsAction("BookmarkBarFolder_DragEnd"));
  }
  return doDrag;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)info {
  if ([[self controller] dragBookmarkData:info])
    return YES;
  NSPasteboard* pboard = [info draggingPasteboard];
  if ([pboard dataForType:kBookmarkButtonDragType] &&
      [self performDragOperationForBookmarkButton:info])
    return YES;
  if ([pboard containsURLData] && [self performDragOperationForURL:info])
    return YES;
  return NO;
}

- (void)setDropIndicatorShown:(BOOL)flag {
  if (dropIndicatorShown_ == flag)
    return;

  dropIndicatorShown_ = flag;
  if (dropIndicatorShown_) {
    NSRect frame = NSInsetRect([self bounds], 4, 0);
    frame.size.height = 1;
    dropIndicator_.reset([[NSBox alloc] initWithFrame:frame]);
    [dropIndicator_ setBoxType:NSBoxSeparator];
    [dropIndicator_ setBorderType:NSLineBorder];
    [dropIndicator_ setAlphaValue:0.85];
    [self addSubview:dropIndicator_];
  } else {
    [dropIndicator_ removeFromSuperview];
    dropIndicator_.reset();
  }
}

@end
