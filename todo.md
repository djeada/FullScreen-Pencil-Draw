# Feature Enhancement Roadmap

This document outlines features to be implemented to enhance FullScreen Pencil Draw, comparing current capabilities against modern collaborative drawing and diagramming applications.

## Feature Comparison Status

### ✅ Currently Implemented Features

| Feature | Status | Notes |
|---------|--------|-------|
| Pen/Pencil Tool | ✅ Implemented | Smooth freehand drawing with Catmull-Rom spline interpolation |
| Eraser Tool | ✅ Implemented | Remove items with visual preview cursor |
| Text Tool | ✅ Implemented | Add text annotations with customizable font size |
| Fill Tool | ✅ Implemented | Fill closed shapes with current color |
| Line Tool | ✅ Implemented | Draw straight lines |
| Arrow Tool | ✅ Implemented | Draw arrows for diagrams and annotations |
| Rectangle Tool | ✅ Implemented | Draw rectangles and squares |
| Circle Tool | ✅ Implemented | Draw circles and ellipses |
| Selection Tool | ✅ Implemented | Select, move, and transform items with rubber-band selection |
| Pan Tool | ✅ Implemented | Navigate around large canvases by dragging |
| Zoom In/Out | ✅ Implemented | Ctrl+Scroll, keyboard shortcuts, or toolbar buttons |
| Grid Overlay | ✅ Implemented | Toggle alignment grid for precise positioning |
| Opacity Control | ✅ Implemented | Adjust brush transparency with slider |
| Brush Size Control | ✅ Implemented | Visual display with +/- buttons and keyboard shortcuts |
| Color Picker | ✅ Implemented | Full color dialog with current color preview |
| Filled Shapes Toggle | ✅ Implemented | Draw filled or outlined rectangles and circles |
| Undo/Redo | ✅ Implemented | Full action history with unlimited undo levels |
| New Canvas | ✅ Implemented | Create custom-sized canvas with background color choice |
| Open/Import Image | ✅ Implemented | Import PNG, JPG, BMP, GIF as background layer |
| Drag-and-Drop Upload | ✅ Implemented | Drag images directly from file system |
| Export to Image | ✅ Implemented | Export to PNG, JPG, or BMP formats |
| Export Selection | ✅ Implemented | Export selected items in SVG, PNG, or JPG formats |
| Copy/Cut/Paste | ✅ Implemented | Full clipboard support for items |
| Duplicate Items | ✅ Implemented | Quick duplicate selected items |
| Keyboard Shortcuts | ✅ Implemented | Comprehensive shortcut support |

---

## 🔧 Features To Implement

### High Priority - Core Collaboration Features

- [ ] **Real-time Collaboration**
  - Enable multiple users to draw and edit on the same canvas simultaneously
  - Implement WebSocket or similar real-time communication protocol
  - Show cursor positions of other users
  - Display user indicators (colored cursors, names)

- [ ] **User Authentication System**
  - Implement user registration and login
  - Support for email/password authentication
  - Optional: OAuth integration (Google, GitHub, etc.)
  - User profile management

- [ ] **Cloud Storage Integration**
  - Save drawings to cloud storage
  - Sync drawings across devices
  - Auto-save functionality
  - Version history for documents

- [ ] **Workspace/Project Management**
  - Create and manage multiple workspaces
  - Organize drawings into projects/folders
  - Recent files list
  - Search functionality for documents

### Medium Priority - Enhanced Drawing Features

- [ ] **Diagram-as-Code Support**
  - Parse text/code syntax to generate diagrams
  - Support for sequence diagrams
  - Support for flowchart diagrams
  - Support for entity-relationship diagrams
  - Support for cloud architecture diagrams

- [ ] **Shape Library**
  - Pre-built shapes for common diagram types
  - Icon library for cloud architecture (AWS, Azure, GCP icons)
  - Custom shape creation and saving
  - Shape categories and search

- [ ] **Connector Tool**
  - Smart connectors between shapes
  - Auto-routing of connector lines
  - Different connector styles (straight, curved, orthogonal)
  - Connector labels

- [ ] **Layer System**
  - Multiple layers support
  - Layer visibility toggle
  - Layer locking
  - Layer reordering

- [ ] **Theme Support**
  - Light theme (current)
  - Dark theme
  - Custom theme creation
  - Per-document theme settings

### Medium Priority - Sharing & Export

- [ ] **Sharing & Permissions**
  - Share drawings via link
  - Set permissions (view only, edit, comment)
  - Team/organization support
  - Public/private document settings

- [ ] **Enhanced Export Options**
  - Export to PDF format
  - Export to SVG (vector) format
  - Export with custom resolution/DPI
  - Export specific layers only
  - Export as embeddable code snippet

- [ ] **Comments & Annotations**
  - Add comments to specific areas of the canvas
  - Reply to comments
  - Resolve/close comments
  - Comment notifications

### Lower Priority - UI/UX Improvements

- [ ] **Responsive/Mobile Design**
  - Touch-friendly interface
  - Mobile-optimized toolbar
  - Gesture support (pinch to zoom, two-finger pan)
  - Tablet stylus optimization

- [ ] **Templates System**
  - Pre-built templates for common use cases
  - User-created template saving
  - Template categories
  - Template sharing

- [ ] **Presentation Mode**
  - Full-screen presentation view
  - Navigate between canvas sections
  - Pointer/laser tool
  - Timer functionality

- [ ] **Keyboard Navigation Enhancement**
  - Vim-style navigation (optional)
  - Command palette (Ctrl+Shift+P style)
  - Customizable keyboard shortcuts

### Lower Priority - Advanced Features

- [ ] **AI-Assisted Features**
  - Auto-layout for diagrams
  - Shape recognition (convert freehand to shapes)
  - Smart text extraction from images
  - Diagram generation from natural language

- [ ] **Import/Integration**
  - Import from other diagram formats (Draw.io, Lucidchart, etc.)
  - Import from Markdown with diagram syntax
  - GitHub/GitLab integration
  - Embed in documentation tools

- [ ] **Offline Support**
  - Full offline functionality
  - Auto-sync when connection restored
  - Conflict resolution for concurrent edits

- [ ] **Plugin/Extension System**
  - API for third-party extensions
  - Custom tool plugins
  - Custom export format plugins
  - Marketplace for plugins

---

## 📋 Implementation Notes

### Architecture Considerations
- Current: C++/Qt6 desktop application
- For real-time collaboration: Consider WebSocket server component
- For cloud storage: Backend service needed (REST API)
- For user auth: Authentication service integration

---

## 🛡️ Stability & Safety Architecture Overhaul (High Priority)

### Goal
Make item lifetime, selection, undo/redo, layers, and overlays **memory-safe** and **consistent** by centralizing ownership and replacing raw-pointer assumptions.

### Phase 0 — Audit + Crash Inventory (1–2 weeks)
- [ ] Create a crash log doc (stack traces + reproduction steps)
- [ ] List every place that creates, removes, or deletes QGraphicsItem
- [ ] Add temporary runtime checks (asserts + qWarning) for invalid item use
- [ ] Build with sanitizers (ASan/UBSan) and run stress scenarios

### Phase 1 — Define the Lifetime Model (Design Doc)
- [ ] Choose a single ownership model (recommended: Item Registry with stable IDs)
- [ ] Document invariants:
  - Only the registry can create/destroy items
  - No subsystem stores raw pointers long-term
  - All deletions are deferred (never during paint)
- [ ] Draft migration plan and compatibility shims

### Phase 2 — Item Registry + Stable IDs (Core Infrastructure)
- [ ] Add `ItemId` type (QUuid or uint64)
- [ ] Create `ItemRegistry` / `ItemStore`:
  - map ItemId -> QGraphicsItem*
  - create/destroy APIs
  - deferred delete queue (QMetaObject::invokeMethod)
- [ ] Introduce a lightweight `ItemRef` handle:
  - stores ItemId
  - resolves through registry on demand
  - returns nullptr if missing

### Phase 3 — Convert Items to Tracked Objects
- [ ] Create tracked item subclasses (e.g., TrackedRectItem, TrackedPathItem)
- [ ] Assign ItemId at creation
- [ ] Ensure registry is notified on destruction
- [ ] Update tools to create items only through registry APIs

### Phase 4 — Replace Raw Pointers in Core Systems
- [ ] Layers store ItemId instead of QGraphicsItem*
- [ ] PDF overlays store ItemId instead of QGraphicsItem*
- [ ] Transform handles store ItemId and resolve safely
- [ ] Selection and export paths use ItemRefs, never raw pointers

### Phase 5 — Undo/Redo Redesign (Snapshot-Based)
- [ ] Replace `Action` item pointers with:
  - ItemId
  - serialized snapshot of item state (geometry + style)
- [ ] Redo re-creates items if missing
- [ ] Undo removes via registry (no direct delete)

### Phase 6 — Centralized Scene Controller
- [ ] Add a SceneController/DocumentController that:
  - owns the registry
  - mediates add/remove/move/modify
  - updates layers/overlays/handles in one place
- [ ] Disallow direct `scene_->addItem/removeItem` outside controller

### Phase 7 — Guard Painting and Movement
- [ ] Defer deletions to avoid paint-time destruction
- [ ] Block re-entrant scene mutations during paint
- [ ] Add guard rails for tools (cancel on deactivation)

### Phase 8 — Test & Validation
- [ ] Add unit tests for registry + ItemRef
- [ ] Add integration tests for delete/undo/redo/selection
- [ ] Stress tests: rapid create/delete, layer deletes, PDF overlay switching
- [ ] Run ASan/UBSan + gdb to confirm no invalid access

### Phase 9 — Documentation & Guidelines
- [ ] Add a “No raw QGraphicsItem* ownership” rule to CONTRIBUTING
- [ ] Document item lifecycle and deletion rules
- [ ] Add diagram of SceneController flow and ownership

### Suggested Technology Stack for New Features
- **Real-time Communication**: WebSocket, SignalR, or Socket.io
- **Cloud Backend**: REST API with PostgreSQL/MongoDB
- **Authentication**: JWT tokens, OAuth 2.0
- **File Storage**: S3-compatible object storage

### Migration Path
1. Start with local-first improvements (themes, layers, shapes)
2. Add export/import capabilities
3. Implement optional cloud backend
4. Enable collaboration features
5. Build mobile/web companion app

---

## 🎯 Quick Wins (Low Effort, High Impact)

These features can be implemented quickly and provide immediate value:

1. [x] **Dark Theme** - Qt styling (QSS/QPalette) changes only
2. [x] **PDF Export** - Use existing Qt PDF capabilities
3. [x] **Recent Files List** - Track and display recently opened files
4. [x] **Auto-save** - Periodically save work locally
5. [x] **Snap to Grid** - Align shapes to grid intersections
6. [x] **Shape Duplication with Offset** - Smart paste at offset position
7. [x] **Ruler/Guides** - Visual guides for alignment
8. [x] **Measurement Tool** - Display distances between objects
9. [x] **Lock Objects** - Prevent accidental modification
10. [ ] **Group/Ungroup** - Combine multiple objects

---

## 📊 Priority Matrix

| Effort \ Impact | High Impact | Medium Impact | Low Impact |
|-----------------|-------------|---------------|------------|
| **Low Effort** | Dark Theme, Auto-save, Recent Files | Lock Objects, Snap to Grid | Ruler/Guides |
| **Medium Effort** | PDF Export, Layer System | Templates, Group/Ungroup | Measurement Tool |
| **High Effort** | Real-time Collaboration, Cloud Storage | Diagram-as-Code, AI Features | Plugin System |

---

## ✅ Acceptance Criteria Checklist

For each feature, ensure:
- [ ] Feature works correctly in all supported scenarios
- [ ] Keyboard shortcuts are consistent with existing patterns
- [ ] Documentation is updated
- [ ] Unit tests are added where applicable
- [ ] Performance is acceptable (no noticeable lag)
- [ ] Accessibility considerations are addressed
- [ ] Error handling is implemented
