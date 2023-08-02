# FullScreen Pencil Draw 

This is a simple yet powerful FullScreen Pencil Draw app built using C++ and the Qt6 framework. It is intended as a demonstration of how to build a graphic editor using Qt6.

## Features

- Draw on a canvas with a customizable brush size and color
- Use tools such as a pen or eraser
- Undo and redo functionality
- Export your creations to PNG or JPG files

## Usage

1. Run the application: `./FullScreen-Pencil-Draw` from the terminal, or by double-clicking the executable.
2. Select the tool you want to use from the toolbar. The available tools are pen and eraser.
3. Click and drag on the canvas to draw. 
4. You can change the color and size of the tool from the toolbar.
5. Use the undo and redo buttons to correct mistakes.
6. Save your creation by clicking the "Save" button, and choose your preferred format (PNG or JPG).

## Building the Application

The application uses CMake as a build system, so make sure you have it installed before proceeding.

Here are the instructions to build the application:

```bash
# Clone the repository
https://github.com/djeada/FullScreen-Pencil-Draw.git
cd FullScreen-Pencil-Draw

# Create a new build directory and navigate into it
mkdir build
cd build

# Generate the build system files with CMake
cmake ..

# Build the application
make
```

Now you should have an executable named FullScreen-Pencil-Draw in the build directory.
