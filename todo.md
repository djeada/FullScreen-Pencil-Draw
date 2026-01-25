# Stability Overhaul Plan

## Objective
Replace the current multi-owner pointer model with a single source of truth for item lifetime and access to eliminate use-after-free, double-delete, and paint-time crashes.

## Problem Statement (Current State)
- Multiple subsystems own or cache `QGraphicsItem*` and mutate/delete items independently.
- Layers, overlays, undo/redo, tools, selection, and transform handles all assume pointer validity.
- Deletions can occur during paint or event handling, causing crashes (e.g., pure-virtual calls in Qt internals).
- There is no consistent lifecycle contract for items.

## Target Architecture (Single Source of Truth)
### Core Concepts
- **ItemId**: Stable identifier for every item (QUuid or uint64).
- **ItemStore**: The only owner of item lifetimes.
- **ItemRef**: A lightweight handle that resolves ItemId -> item at use time.
- **SceneController**: The only path for add/remove/move/modify operations.

### Invariants (Non‑negotiable)
- Only the ItemStore creates or destroys items.
- No subsystem stores raw `QGraphicsItem*` long-term.
- All scene mutations go through SceneController/ItemStore APIs.
- Deletion is queued/deferred (never executed during paint/event handling).
- Undo/redo operates on ItemId + snapshot, never raw pointers.

## Deliverables
1) ItemStore + ItemId + ItemRef core implemented.
2) SceneController introduced and enforced.
3) Layers and PDF overlays converted to ItemId storage.
4) Undo/redo rewritten to snapshot-based actions.
5) Tools refactored to create items only via ItemStore.
6) All raw-pointer usage removed from long-lived structures.
7) Safety tests + sanitizer passes added.

## Task Plan (Detailed)
### Phase 1 — Design + Contracts
- [x] Write a short design doc describing ItemStore/ItemRef/SceneController APIs.
- [x] Define the exact deletion semantics (deferred queue + flush point).
- [x] Document invariants and add them to CONTRIBUTING.

### Phase 2 — Core Infrastructure
- [x] Implement ItemId type.
- [x] Implement ItemStore:
  - create/destroy APIs
  - map ItemId -> QGraphicsItem*
  - deferred deletion queue
- [x] Implement ItemRef resolver with null-on-missing behavior.

### Phase 3 — SceneController
- [x] Add SceneController as the only entry point for add/remove/move/modify.
- [ ] Replace direct `scene_->addItem/removeItem` calls with controller calls.
- [ ] Add guard rails (assert on direct scene mutations outside controller).

### Phase 4 — Convert Subsystems
- [x] Layers store ItemId instead of `QGraphicsItem*`.
- [x] PDF overlays store ItemId instead of `QGraphicsItem*`.
- [x] Transform handles store ItemId and resolve on use.
- [ ] Selection and export paths use ItemRef resolution only.

### Phase 5 — Undo/Redo Redesign
- [x] Replace Action item pointers with snapshots + ItemId.
- [x] Redo recreates items if missing.
- [x] Undo removes via ItemStore (no raw deletes).

### Phase 6 — Tools Refactor
- [ ] Tools create items via ItemStore APIs only.
- [ ] Tools store ItemId for in-progress items (no raw pointer caching).
- [ ] Cancel/cleanup on tool deactivate through SceneController.

### Phase 7 — Validation
- [ ] Add targeted tests: erase/undo/redo, layer delete, overlay switch.
- [ ] Add stress tests: rapid create/delete/move and repaint.
- [ ] Run ASan/UBSan; confirm no invalid memory access.

## Notes
This plan replaces the entire prior roadmap. The focus is to stabilize core architecture before adding features.
