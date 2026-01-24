# FullScreen Pencil Draw

A drawing application built with C++ and Qt6 for creating sketches and illustrations.

## Screenshots

![Application Screenshot](https://github.com/user-attachments/assets/0e311731-69f6-458a-bcc9-fb2a4d46b32b)

## Features

### Drawing Tools
- **Pen**: Freehand drawing with Catmull-Rom spline interpolation
- **Eraser**: Remove items with preview cursor
- **Text**: Add text annotations
- **Fill**: Fill closed shapes
- **Line**: Draw straight lines
- **Arrow**: Draw arrows
- **Rectangle**: Draw rectangles and squares
- **Circle**: Draw circles and ellipses

### PDF Viewing & Annotation
- Open multi-page PDF documents
- Navigate between pages with toolbar or keyboard shortcuts
- PDF content renders as read-only background
- Draw annotations on top of PDF pages
- Each page maintains separate overlay annotations
- Undo/redo support per page
- Toggle dark mode for color inversion
- Export annotated PDF

### Navigation & Selection
- **Selection**: Select, move, and transform items with rubber-band selection
- **Pan**: Navigate around the canvas by dragging
- **Zoom**: Zoom with Ctrl+Scroll, keyboard shortcuts, or toolbar buttons
- **Grid**: Toggle alignment grid

### Additional Features
- **Layers**: Layer management with visibility, opacity, and lock controls
- **Brush Preview**: Visual preview of brush size and color
- **Opacity Control**: Adjust brush transparency
- **Brush Size**: Adjust with +/- buttons and keyboard shortcuts
- **Color Picker**: Color selection dialog
- **Filled Shapes**: Toggle filled or outlined shapes (B key)
- **Zoom Display**: Real-time zoom percentage
- **Cursor Position**: Live X/Y coordinates
- **Undo/Redo**: Unlimited undo levels
- **Clear Canvas**: Canvas reset with confirmation dialog

### Layer Management
- Dockable panel for layer organization
- Add and delete layers
- Toggle layer visibility
- Lock layers to prevent edits
- Adjust layer opacity
- Reorder layers in stack
- Duplicate layers
- Merge layers

### File Operations
- **New Canvas**: Create custom-sized canvas with background color
- **Open Image**: Import PNG, JPG, BMP, GIF as background
- **Open PDF**: Load PDF documents for annotation (requires Qt PDF module)
- **Drag-and-Drop**: Drag images from file system with dimension dialog
- **Save/Export**: Export to PNG, JPG, or BMP
- **Export Selection**: Right-click selected items to export as SVG, PNG, or JPG
- **Export PDF**: Save annotated PDFs
- **Clear Canvas**: Reset to blank state

### Edit Operations
- Copy, cut, and paste items
- Duplicate selected items (Ctrl+D)
- Delete selected items
- Select all items
- Export selected items via context menu (SVG, PNG, JPG)

## Keyboard Shortcuts

| Category | Shortcut | Action |
|----------|----------|--------|
| **Tools** | `P` | Pen tool |
| | `E` | Eraser tool |
| | `T` | Text tool |
| | `F` | Fill tool |
| | `L` | Line tool |
| | `A` | Arrow tool |
| | `R` | Rectangle tool |
| | `C` | Circle tool |
| | `S` | Selection tool |
| | `H` | Pan/Hand tool |
| **View** | `G` | Toggle grid |
| | `B` | Toggle filled shapes |
| | `+` / `=` | Zoom in |
| | `-` | Zoom out |
| | `0` | Reset zoom (1:1) |
| | `Ctrl+Scroll` | Zoom with mouse |
| **Brush** | `[` | Decrease brush size |
| | `]` | Increase brush size |
| **Edit** | `Ctrl+Z` | Undo |
| | `Ctrl+Y` | Redo |
| | `Ctrl+C` | Copy |
| | `Ctrl+X` | Cut |
| | `Ctrl+V` | Paste |
| | `Ctrl+D` | Duplicate |
| | `Ctrl+A` | Select all |
| | `Delete` | Delete selected |
| **File** | `Ctrl+N` | New canvas |
| | `Ctrl+O` | Open image |
| | `Ctrl+Shift+O` | Open PDF |
| | `Ctrl+S` | Save |
| | `Esc` | Exit |
| **PDF Navigation** | `Page Down` | Next page |
| | `Page Up` | Previous page |
| | `Home` | First page |
| | `End` | Last page |

## Architecture

The application uses a Model-View architecture:
- **Canvas**: QGraphicsView-based drawing surface
- **ToolPanel**: Modular toolbar with tool switching
- **LayerManager**: Centralized layer management
- **LayerPanel**: Dockable widget for layer manipulation
- **Action System**: Undo/redo stack with polymorphic actions
- **Tool System**: Extensible tool architecture with base Tool class
- **PdfViewer**: PDF viewing and annotation widget
- **PdfDocument**: PDF loading with page caching
- **PdfOverlayManager**: Per-page overlay management
- Built with C++17 features

## Usage

1. **Launch the Application**

```bash
./FullScreen-Pencil-Draw
```

2. **Select a Tool**

Use the toolbar or keyboard shortcuts to select drawing tools.

3. **Draw on the Canvas**

Click and drag on the canvas to draw. Adjust color and brush size from the toolbar.

4. **Undo and Redo**

Use Ctrl+Z to undo and Ctrl+Y to redo actions.

5. **Save Your Work**

Use Ctrl+S or the Save button to export your drawing as PNG, JPG, or BMP.

6. **Import Images**

Drag and drop image files onto the canvas. A dialog will appear to adjust dimensions.

## Building the Application

### Prerequisites

- C++ compiler with C++17 support (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.16 or higher
- Qt6 base development packages
- OpenGL development libraries

### Build Steps

1. **Clone the Repository**

```bash
git clone https://github.com/djeada/FullScreen-Pencil-Draw.git
cd FullScreen-Pencil-Draw
```

2. **Install Dependencies**

On Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake qt6-base-dev qt6-tools-dev qt6-tools-dev-tools libgl1-mesa-dev
```

Refer to Qt documentation for other operating systems.

3. **Create Build Directory**

```bash
mkdir build
cd build
```

4. **Configure with CMake**

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

5. **Build**

```bash
make -j$(nproc)
```

6. **Run**

```bash
./FullScreen-Pencil-Draw
```

## Troubleshooting

- Ensure all dependencies are installed
- Verify Qt6 is accessible by CMake
- Check build logs for error messages
- Report issues on [GitHub](https://github.com/djeada/FullScreen-Pencil-Draw/issues)

## Contributing

Contributions are welcome. Open an issue or submit a pull request.

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Contact

For questions or feedback, use [GitHub Issues](https://github.com/djeada/FullScreen-Pencil-Draw/issues).

