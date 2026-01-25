# TODO

This document tracks the implementation status of enforcing ItemStore as the sole source of truth for item lifetime.

## Completed ✓

- [x] **ItemStore as central registry** - Implemented in `item_store.h/cpp` with unique ItemId assignment, deferred deletion, and safe lookup.

- [x] **ItemId for stable references** - Implemented in `item_id.h` with UUID-based stable identifiers that remain valid across undo/redo.

- [x] **ItemRef for safe point-of-use resolution** - Implemented in `item_ref.h` to replace raw `QGraphicsItem*` storage.

- [x] **SceneController as central controller** - Implemented in `scene_controller.h/cpp`. All tools (pen, shape, text, eraser, arrow, fill) use SceneController when available.

- [x] **itemAboutToBeDeleted signal** - Implemented in ItemStore and connected by Canvas to clear transform handle cached IDs.

- [x] **Transform handles subscribe to deletion** - Canvas connects to `itemAboutToBeDeleted` and removes/clears TransformHandleItems for deleted items.

- [x] **Tools use ItemId for tracking** - PenTool, ShapeTool, TextTool, ArrowTool all maintain ItemId alongside cached pointers and resolve at point-of-use. EraserTool's preview is a UI helper, not registered with ItemStore.

- [x] **Undo/redo actions use ItemId** - DrawAction, DeleteAction, MoveAction, FillAction, TransformAction, GroupAction, UngroupAction all support ItemId-based resolution.

- [x] **Layer system uses ItemId** - Layer class tracks items by ItemId, resolves via ItemStore, and cleans up stale references.

- [x] **Pointer safety tests** - Comprehensive tests in `test_item_store.cpp` verify:
  - `itemAboutToBeDeleted` signal is properly emitted
  - ItemRef becomes invalid after deletion
  - Subscribers can clear cached IDs on deletion notification
  - Code paths tolerate missing items (return nullptr, don't crash)
  - SceneController handles removal of deleted items gracefully
  - ItemRef works correctly after item restoration (undo)

## Notes

### UI Helper Items (NOT registered with ItemStore)

The following are UI helper items that should **NOT** be registered with ItemStore. 
Registering them would cause `SceneController::clearAll()` to delete them while tools/widgets
still hold raw pointers, leading to dangling pointer crashes.

1. **TransformHandleItems** - UI overlay for resize/rotate handles on selected items.
2. **EraserTool::eraserPreview_** - UI cursor preview showing eraser size (in both Canvas and EraserTool).
3. **backgroundImage_** - Special singleton item for background images.
4. **PdfPageItem** - PDF page display (not user-drawn content).

### Acceptable direct scene access

Actions for undo/redo may need direct scene access for legacy compatibility, but they use
ItemStore-based resolution when available.

### Pattern for new code

When adding new tools or widgets that track **user content** items:

1. Store `ItemId` (not raw `QGraphicsItem*`) for any long-lived reference
2. Resolve via `itemStore()->item(id)` or `ItemRef` only at point-of-use
3. Subscribe to `itemAboutToBeDeleted` if caching any ItemIds
4. Check for `nullptr` before using resolved items

When adding **UI helper items** (previews, handles, overlays):

1. Do **NOT** register with ItemStore via `registerItem()` or `SceneController::addItem()`
2. Add directly to scene via `scene()->addItem()` or `scene()->addEllipse()` etc.
3. Manage lifecycle manually - scene will clean up when destroyed

