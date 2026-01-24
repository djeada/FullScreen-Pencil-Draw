# FullScreen Pencil Draw

A vector and raster graphics editor built with C++ and Qt6. This application provides drawing tools for quick sketches and detailed illustrations.

## Screenshots

![Application Screenshot](https://github.com/user-attachments/assets/0e311731-69f6-458a-bcc9-fb2a4d46b32b)

## Features

### Drawing Tools
- **Pen Tool**: Freehand drawing with Catmull-Rom spline interpolation
- **Eraser Tool**: Remove items with preview cursor
- **Text Tool**: Add text annotations with customizable font size
- **Fill Tool**: Fill closed shapes with current color
- **Line Tool**: Draw straight lines
- **Arrow Tool**: Draw arrows for diagrams and annotations
- **Rectangle Tool**: Draw rectangles and squares
- **Circle Tool**: Draw circles and ellipses

### PDF Viewing & Annotation
- **Open PDF Files**: Load multi-page PDF documents for viewing and annotation
- **Multi-Page Navigation**: Navigate between pages with toolbar buttons or keyboard shortcuts
- **PDF Background**: PDF content is rendered as a read-only background layer
- **Overlay Drawing**: Draw annotations on top of PDF pages using all drawing tools
- **Per-Page Overlays**: Each page maintains its own overlay annotations
- **Undo/Redo Per Page**: Undo/redo support isolated to each page's overlays
- **Dark Mode**: Toggle color inversion for viewing in low-light environments
- **Export Annotated PDF**: Export the annotated document to a new PDF file

### Navigation & Selection
- **Selection Tool**: Select, move, and transform items with rubber-band selection
- **Pan Tool**: Navigate around the canvas by dragging
- **Zoom**: Zoom in/out with Ctrl+Scroll, keyboard shortcuts, or toolbar buttons
- **Grid Overlay**: Toggle alignment grid for positioning

### Core Features
- **Layer System**: Layer management with visibility, opacity, and lock controls
- **Brush Preview**: Visual preview widget showing brush size and color
- **Opacity Control**: Adjust brush transparency with slider
- **Brush Size Control**: Visual display with +/- buttons and keyboard shortcuts
- **Color Picker**: Color dialog with current color preview
- **Filled Shapes Toggle**: Draw filled or outlined rectangles and circles (B key)
- **Zoom Level Display**: Real-time zoom percentage indicator
- **Cursor Position**: Live X/Y coordinate display
- **Undo/Redo**: Action history with unlimited undo levels (including fill operations)
- **Clear Canvas Confirmation**: Confirmation dialog to prevent accidental canvas clearing

### Layer System
- **Layer Panel**: Dockable panel for layer management
- **Add/Delete Layers**: Create new layers or remove existing ones
- **Layer Visibility**: Toggle eye icon to show/hide layers
- **Layer Lock**: Lock layers to prevent edits
- **Layer Opacity**: Adjust individual layer transparency
- **Layer Ordering**: Move layers up/down in the stack
- **Duplicate Layers**: Create copies of existing layers
- **Merge Down**: Merge a layer with the one below it

### File Operations
- **New Canvas**: Create custom-sized canvas with background color choice
- **Open Image**: Import PNG, JPG, BMP, GIF as background layer
- **Open PDF**: Load PDF documents for annotation (requires Qt PDF module)
- **Drag-and-Drop Upload**: Drag images directly from file system with dimension specification dialog
- **Save/Export**: Export to PNG, JPG, or BMP formats
- **Export Selection**: Right-click on selected items to export in SVG, PNG, or JPG formats
- **Export Annotated PDF**: Save annotated PDF documents to new files
- **Clear Canvas**: Reset to blank state

### Edit Operations
- **Copy/Cut/Paste**: Clipboard support for items
- **Duplicate**: Duplicate selected items (Ctrl+D)
- **Delete**: Remove selected items
- **Select All**: Select all canvas items
- **Context Menu Export**: Right-click on selected items to export as SVG, PNG, or JPG

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
- **Canvas**: QGraphicsView-based drawing surface with scene management
- **ToolPanel**: Modular toolbar with signal/slot-based tool switching  
- **LayerManager**: Centralized layer management
- **LayerPanel**: Dockable widget for layer manipulation
- **Action System**: Undo/redo stack with polymorphic action classes
- **Tool System**: Extensible tool architecture with base Tool class
- **PdfViewer**: PDF viewing and annotation widget using Qt PDF module
- **PdfDocument**: PDF document loading with page caching
- **PdfOverlayManager**: Per-page overlay management for annotations
- Built with C++17 features

## Usage

1. **Launch the Application**

```bash
./FullScreen-Pencil-Draw
```

You can also launch the application by double-clicking the `FullScreen-Pencil-Draw` executable in the build directory.

2. **Select a Tool**

Use the toolbar or keyboard shortcuts to select drawing tools, eraser, or other functions.

3. **Draw on the Canvas**

Click and drag your mouse (or use a stylus) on the canvas to draw. Change the color and size of your selected tool from the toolbar to customize your drawing.

4. **Undo and Redo Actions**

If you make a mistake, use the Undo button or Ctrl+Z to revert the last action. Use Redo or Ctrl+Y to reapply it.

5. **Save Your Artwork**

Click the Save button on the toolbar or use Ctrl+S. Select your preferred image format (PNG, JPG, or BMP) and choose the destination folder to save your creation.

6. **Import Images via Drag-and-Drop**

Drag an image file (PNG, JPG, JPEG, BMP, or GIF) from your file system and drop it anywhere on the canvas. A dialog will appear allowing you to specify the desired dimensions. Adjust the width and height (aspect ratio is maintained by default), then click OK to add the image to your canvas at the drop location. The imported image can be selected, moved, resized, and edited like any other canvas item.

## Building the Application

### Prerequisites

- C++ compiler with C++17 support (GCC 7+, Clang 5+, MSVC 2017+)
- CMake version 3.16 or higher
- Qt6 base development packages and tools
- OpenGL development libraries
- Xvfb (optional, for running GUI applications in a headless environment)

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
sudo apt-get install -y build-essential cmake qt6-base-dev qt6-tools-dev qt6-tools-dev-tools libgl1-mesa-dev xvfb
```

Refer to the official Qt documentation for installation instructions on other operating systems.

3. **Create a Build Directory**

```bash
mkdir build
cd build
```

4. **Configure with CMake**

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

5. **Build the Application**

```bash
make -j$(nproc)
```

The `-j$(nproc)` flag enables parallel compilation using all available CPU cores.

6. **Run the Application**

```bash
./FullScreen-Pencil-Draw
```

You can also double-click the executable in the `build` directory.

## Troubleshooting

If you encounter issues while building or running the application:

- Ensure all required dependencies are installed and up to date
- Verify Qt6 is correctly installed and accessible by CMake
- Check the output logs for error messages that can guide you in resolving issues
- Open an issue on the [GitHub repository](https://github.com/djeada/FullScreen-Pencil-Draw/issues) with details about the problem

## Contributing

Contributions are welcome. If you have suggestions, improvements, or bug fixes, open an issue or submit a pull request.

### How to Contribute

1. Fork the repository
2. Create a new branch: `git checkout -b feature/YourFeatureName`
3. Make your changes
4. Commit your changes: `git commit -m "Add Your Feature Description"`
5. Push to your fork: `git push origin feature/YourFeatureName`
6. Open a pull request

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Contact

For questions or feedback, use [GitHub Issues](https://github.com/djeada/FullScreen-Pencil-Draw/issues).

