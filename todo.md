# TODO
- Enforce ItemStore as the sole source of truth for item lifetime and access across the entire app. This means:
  - No subsystem/tool/widget is allowed to keep long-lived raw `QGraphicsItem*` (previews, handles, overlays, selection, undo/redo, etc.).
  - No direct `scene()->addItem/removeItem` outside SceneController/ItemStore. All create/delete/move/modify must go through SceneController.
  - Every item must have an ItemId; all references must be ItemId/ItemRef, resolved only at point-of-use.
  - Any cached UI helpers (eraser preview, transform handles, overlay items) must subscribe to `itemAboutToBeDeleted` and clear/invalidate their stored IDs so they never touch freed memory.
  - Any code path that can outlive a deletion must tolerate missing items (null from ItemRef) and bail without dereference.
  - Add checks/tests to fail if a raw pointer is stored long-term or if a tool bypasses the controller.
