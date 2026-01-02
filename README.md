# FullScreen Pencil Draw

FullScreen Pencil Draw is a simple yet powerful application built with C++ and the Qt6 framework. Designed as a demonstration of creating a graphic editor using Qt6, this app allows users to draw on a full-screen canvas with customizable tools and export their creations effortlessly.

## Screenshots

![Application Screenshot](https://github.com/user-attachments/assets/73c98ec3-6d65-4752-9341-5282b34841ad)

## Features

- **Drawing Tools**: Freehand pen, line, rectangle, and circle tools
- **Brush Customization**: Adjust thickness and color of your brush
- **Eraser Tool**: Remove unwanted parts of your drawing
- **Selection Tool**: Select, move, copy, cut, and paste items
- **Undo/Redo**: Easily revert or reapply changes
- **Zoom**: Zoom in/out with mouse wheel (Ctrl+Scroll) or toolbar buttons
- **Save/Export**: Save your creations in PNG or JPG format
- **Full Screen**: Utilize the entire screen for an immersive experience
- **Status Bar**: Shows current keyboard shortcuts
- **Tooltips**: Hover over tools to see their function and shortcuts

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `P` | Pen tool |
| `E` | Eraser tool |
| `L` | Line tool |
| `R` | Rectangle tool |
| `C` | Circle tool |
| `S` | Selection tool |
| `K` | Color picker |
| `[` | Decrease brush size |
| `]` | Increase brush size |
| `+` / `=` | Zoom in |
| `-` | Zoom out |
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `Ctrl+S` | Save |
| `Ctrl+C` | Copy selected items |
| `Ctrl+X` | Cut selected items |
| `Ctrl+V` | Paste |
| `Ctrl+Scroll` | Zoom with mouse wheel |
| `Esc` | Exit application |

## Future Enhancements

- Adding text tool for annotations
- Multiple layers support for complex drawings
- Fill tool for coloring shapes
- Custom brush shapes
- Cross-platform testing and optimization

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

