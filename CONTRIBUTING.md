# Contributing to FullScreen-Pencil-Draw

Thank you for your interest in contributing to FullScreen-Pencil-Draw! This document provides guidelines for contributing, with a focus on the core architectural patterns that ensure application stability.

## Quick Start

1. Fork the repository
2. Create a new branch: `git checkout -b feature/YourFeatureName`
3. Make your changes
4. Commit your changes: `git commit -m "Add Your Feature Description"`
5. Push to your fork: `git push origin feature/YourFeatureName`
6. Open a pull request

## Architecture Overview

### Core Stability Architecture

The application uses a **Single Source of Truth** pattern for item lifecycle management. This architecture eliminates use-after-free, double-delete, and paint-time crashes by enforcing strict ownership and access rules.

#### Key Components

| Component | File | Responsibility |
|-----------|------|----------------|
| **ItemId** | `src/core/item_id.h` | Stable unique identifier for every graphics item |
| **ItemStore** | `src/core/item_store.h/cpp` | Single owner of item lifetimes |
| **ItemRef** | `src/core/item_ref.h` | Lightweight handle that resolves ItemId → item at use time |
| **SceneController** | `src/core/scene_controller.h/cpp` | Single entry point for add/remove/move/modify operations |

### Design Document: ItemStore/ItemRef/SceneController APIs

#### ItemId

`ItemId` is a stable identifier (wrapper around `QUuid`) for every graphics item:

```cpp
// Generate a new unique ID
ItemId id = ItemId::generate();

// Check validity
if (id.isValid()) {
    // Use the ID
}

// Convert to/from string (for serialization)
QString str = id.toString();
ItemId restored = ItemId::fromString(str);
```

**Properties:**
- Immutable value type
- Survives item deletion (for undo/redo)
- Can be compared without accessing memory
- Serializable for save/load operations

#### ItemStore

`ItemStore` is the central registry and owner of all graphics items:

```cpp
// Register an item (assigns ItemId, adds to scene)
ItemId id = itemStore->registerItem(item);

// Look up an item
QGraphicsItem *item = itemStore->item(id);

// Schedule deletion (deferred, safe)
itemStore->scheduleDelete(id, keepForUndo);

// Flush deletions at safe point
itemStore->flushDeletions();

// Restore for undo
itemStore->restoreItem(id);
```

**Invariants:**
- Only ItemStore creates/destroys items
- Items are looked up by ID, never cached as raw pointers
- Deletion is deferred to prevent use-after-free during event handling

#### ItemRef

`ItemRef` is a lightweight handle for safe item access:

```cpp
// Create a reference
ItemRef ref(store, itemId);

// Resolve at use time (returns nullptr if deleted)
if (QGraphicsItem *item = ref.get()) {
    // Safe to use
}

// Typed access
if (auto *rect = ref.getAs<QGraphicsRectItem>()) {
    // Work with rectangle
}

// Validity check
if (ref.isValid()) {
    // Item exists
}
```

**Usage Pattern:**
- Store ItemRef instead of `QGraphicsItem*` in long-lived structures
- Resolve to pointer only when actually using the item
- Always check for nullptr after resolution

#### SceneController

`SceneController` is the single entry point for all scene mutations:

```cpp
// Add an item
ItemId id = controller->addItem(item);

// Remove an item (deferred deletion)
controller->removeItem(id, keepForUndo);

// Restore for undo
controller->restoreItem(id);

// Move an item
controller->moveItem(id, newPos);

// Get safe reference
ItemRef ref = controller->ref(id);
```

### Deletion Semantics

**Deferred Deletion Queue:**

1. When an item needs to be deleted, call `SceneController::removeItem()` or `ItemStore::scheduleDelete()`
2. The item is immediately removed from the scene (safe during event handling)
3. The item is placed in a deletion queue
4. At a safe point (after event handling), `flushDeletions()` is called to actually delete items

**Flush Points:**
- End of event loop iteration (via `QTimer::singleShot(0, ...)`)
- Before scene repaint (if needed)
- Explicitly by calling `flushDeletions()`

**Undo/Redo Integration:**
- When `keepForUndo=true`, the item is moved to snapshot storage instead of the deletion queue
- Undo operations call `restoreItem()` to bring the item back
- Snapshot storage is cleared when undo history is truncated

## Architectural Invariants (Non-Negotiable)

These invariants MUST be maintained in all contributions:

### 1. Single Owner for Item Lifetimes
```
✅ DO: Use ItemStore to create/destroy items
❌ DON'T: Call `delete item` directly
❌ DON'T: Store QGraphicsItem* in member variables long-term
```

### 2. No Raw Pointer Storage
```
✅ DO: Store ItemId or ItemRef in classes
✅ DO: Resolve ItemRef.get() at point of use
❌ DON'T: Cache QGraphicsItem* across function calls
❌ DON'T: Store QGraphicsItem* in containers
```

### 3. Scene Mutations Through Controller
```
✅ DO: Use SceneController::addItem/removeItem
❌ DON'T: Call scene_->addItem/removeItem directly
❌ DON'T: Delete items from scene without going through controller
```

### 4. Deferred Deletion
```
✅ DO: Schedule deletions via SceneController::removeItem
✅ DO: Let the controller flush deletions at safe points
❌ DON'T: Delete items during paint or event handling
❌ DON'T: Delete items while iterating over scene items
```

### 5. Undo/Redo Uses Snapshots
```
✅ DO: Use ItemId + snapshots in Action classes
✅ DO: Recreate items via ItemStore on redo if missing
❌ DON'T: Store raw pointers in Action classes
❌ DON'T: Assume pointers are valid after undo/redo
```

## Code Style Guidelines

### C++ Standards
- Use C++17 features where appropriate
- Prefer `auto` for complex type declarations
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) for ownership
- Prefer references over pointers when null is not valid

### Qt Conventions
- Use Qt naming conventions (camelCase for methods, PascalCase for classes)
- Prefer `QPointer` for cross-object references to QObjects
- Use signals/slots for loose coupling
- Document public APIs with Doxygen-style comments

### Error Handling
- Check for null pointers before dereferencing
- Use assertions for programming errors (invariant violations)
- Log warnings for recoverable issues
- Fail gracefully when possible

## Testing Guidelines

### Required Test Coverage
- Erase/undo/redo cycles
- Layer delete with items
- Overlay switch operations
- Rapid create/delete/move sequences

### Sanitizer Testing
Before submitting a PR, run with sanitizers if possible:
```bash
# AddressSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" ..
make && ./FullScreen-Pencil-Draw

# UndefinedBehaviorSanitizer  
cmake -DCMAKE_CXX_FLAGS="-fsanitize=undefined" ..
make && ./FullScreen-Pencil-Draw
```

## Pull Request Checklist

- [ ] Code follows the architectural invariants
- [ ] No raw `QGraphicsItem*` stored in long-lived structures
- [ ] All scene mutations go through SceneController
- [ ] No direct item deletion (use scheduleDelete)
- [ ] New Action classes use ItemId, not raw pointers
- [ ] Public APIs are documented
- [ ] Tests added for new functionality
- [ ] Existing tests pass
- [ ] No sanitizer warnings

## Questions?

If you have questions about the architecture or how to implement something, please open an issue for discussion before submitting a PR.
