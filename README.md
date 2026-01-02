# FullScreen Pencil Draw

FullScreen Pencil Draw is a professional-grade vector and raster graphics editor built with C++ and Qt6. Designed for both quick sketches and detailed illustrations, this application provides an intuitive mix of drawing tools with a modern, extensible architecture.

## Screenshots

![Application Screenshot](https://github.com/user-attachments/assets/0e311731-69f6-458a-bcc9-fb2a4d46b32b)

## Features

### Drawing Tools
- **Pen Tool**: Smooth freehand drawing with Catmull-Rom spline interpolation
- **Eraser Tool**: Remove items with visual preview cursor
- **Text Tool**: Add text annotations with customizable font size
- **Fill Tool**: Fill closed shapes with current color
- **Line Tool**: Draw straight lines
- **Arrow Tool**: Draw arrows for diagrams and annotations
- **Rectangle Tool**: Draw rectangles and squares
- **Circle Tool**: Draw circles and ellipses

### Navigation & Selection
- **Selection Tool**: Select, move, and transform items with rubber-band selection
- **Pan Tool**: Navigate around large canvases by dragging
- **Zoom**: Zoom in/out with Ctrl+Scroll, keyboard shortcuts, or toolbar buttons
- **Grid Overlay**: Toggle alignment grid for precise positioning

### Professional Features
- **Opacity Control**: Adjust brush transparency with slider
- **Brush Size Control**: Visual display with +/- buttons and keyboard shortcuts
- **Color Picker**: Full color dialog with current color preview
- **Zoom Level Display**: Real-time zoom percentage indicator
- **Cursor Position**: Live X/Y coordinate display
- **Undo/Redo**: Full action history with unlimited undo levels

### File Operations
- **New Canvas**: Create custom-sized canvas with background color choice
- **Open Image**: Import PNG, JPG, BMP, GIF as background layer
- **Save/Export**: Export to PNG, JPG, or BMP formats
- **Export Selection**: Right-click on selected items to export in SVG, PNG, or JPG formats
- **Clear Canvas**: Reset to blank state

### Edit Operations
- **Copy/Cut/Paste**: Full clipboard support for items
- **Duplicate**: Quick duplicate selected items (Ctrl+D)
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
| | `Ctrl+S` | Save |
| | `Esc` | Exit |

## Architecture

The application follows a clean Model-View architecture:
- **Canvas**: QGraphicsView-based drawing surface with scene management
- **ToolPanel**: Modular toolbar with signal/slot-based tool switching  
- **Action System**: Undo/redo stack with polymorphic action classes
- **Modern C++17**: Smart pointers, constexpr, structured bindings

## Usage

Follow these steps to start creating with FullScreen Pencil Draw:

I. **Launch the Application**

```bash

./FullScreen-Pencil-Draw
```

Alternatively, you can launch the application by double-clicking the `FullScreen-Pencil-Draw` executable in the build directory.

II. **Select a Tool**

- For drawing lines and shapes.
- For removing unwanted parts of your drawing.

III. **Draw on the Canvas**

- Click and drag your mouse (or use a stylus) on the canvas to draw.
- Change the color and size of your selected tool directly from the toolbar to customize your drawing.

IV. **Undo and Redo Actions**

- If you make a mistake, click the "Undo" button to revert the last action.
- If you undo an action by mistake, use the "Redo" button to reapply it.

V. **Save Your Artwork**

- Click the "Save" button located on the toolbar.
- Select your preferred image format (PNG or JPG) and choose the destination folder to save your creation.

## Building the Application

To build FullScreen Pencil Draw, ensure you have the necessary dependencies installed and follow the steps below.

### Prerequisites

- A C++ compiler that supports C++17 or later (GCC 7+, Clang 5+, MSVC 2017+)
- CMake version 3.16 or higher
- Qt6 base development packages and tools
- OpenGL development libraries
- Xvfb (optional, for running GUI applications in a headless environment)

### Installation Steps

I. **Clone the Repository**

```bash
git clone https://github.com/djeada/FullScreen-Pencil-Draw.git
cd FullScreen-Pencil-Draw
```

II. **Install Dependencies**

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake qt6-base-dev qt6-tools-dev qt6-tools-dev-tools libgl1-mesa-dev xvfb
```

Refer to the official Qt documentation for installation instructions on your operating system.

III. **Create a Build Directory**

```bash
mkdir build
cd build
```

IV. **Generate Build System Files with CMake**

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

V. **Build the Application**

```bash
make -j$(nproc)
```

The `-j$(nproc)` flag enables parallel compilation using all available CPU cores.

VI. **Run the Application**

```bash

./FullScreen-Pencil-Draw
```

Alternatively, double-click the executable in the `build` directory.

## Troubleshooting

If you encounter issues while building or running the application, consider the following steps:

I. Ensure all required dependencies are installed and up to date.

II. Make sure Qt6 is correctly installed and accessible by CMake.

III. Look at the output logs for any error messages that can guide you in resolving issues.

IV. Open an issue on the [GitHub repository](https://github.com/djeada/FullScreen-Pencil-Draw/issues) detailing the problem you’re facing.

## Contributing

Contributions are welcome! If you have suggestions, improvements, or bug fixes, feel free to open an issue or submit a pull request.

### How to Contribute

I. **Fork the Repository**

II. **Create a New Branch**

```bash
git checkout -b feature/YourFeatureName
```

III. **Make Your Changes**

IV. **Commit Your Changes**

```bash
git commit -m "Add Your Feature Description"
```

V. **Push to Your Fork**

```bash
git push origin feature/YourFeatureName
```

VI. **Open a Pull Request**

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Contact

For any questions or feedback, please reach out via [GitHub Issues](https://github.com/djeada/FullScreen-Pencil-Draw/issues) or contact the maintainer directly.

