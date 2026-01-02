# Drag-and-Drop File Upload Feature

## Overview
This feature enables users to drag and drop image files directly from their file system into the FullScreen Pencil Draw application. Upon dropping a file, a popup menu appears allowing users to specify the desired dimensions before the file is imported.

## Implementation Details

### New Files Created

#### 1. `src/widgets/image_size_dialog.h`
- Custom QDialog for specifying image dimensions
- Provides spinboxes for width and height
- Includes "Maintain aspect ratio" checkbox
- Displays original image dimensions

#### 2. `src/widgets/image_size_dialog.cpp`
- Implements the ImageSizeDialog functionality
- Handles aspect ratio calculations
- Prevents circular updates when aspect ratio is maintained
- Validates input ranges (1-10,000 pixels)

### Modified Files

#### 1. `src/widgets/canvas.h`
- Added drag-and-drop event handler declarations:
  - `dragEnterEvent(QDragEnterEvent *event)`
  - `dragMoveEvent(QDragMoveEvent *event)`
  - `dropEvent(QDropEvent *event)`
- Added helper method: `loadDroppedImage(const QString &filePath, const QPointF &dropPosition)`
- Added necessary includes for drag-and-drop events

#### 2. `src/widgets/canvas.cpp`
- Enabled drag-and-drop with `setAcceptDrops(true)` in constructor
- Implemented drag-and-drop event handlers:
  - `dragEnterEvent`: Accepts drag operations with file URLs
  - `dragMoveEvent`: Provides visual feedback during drag
  - `dropEvent`: Handles file drop and filters for image formats
- Implemented `loadDroppedImage`:
  - Loads the dropped image file
  - Shows ImageSizeDialog for dimension specification
  - Scales image to specified dimensions
  - Adds scaled image to canvas at drop location
  - Makes image selectable and movable
  - Integrates with undo/redo system

## Features

### Supported File Formats
- PNG (.png)
- JPEG (.jpg, .jpeg)
- BMP (.bmp)
- GIF (.gif)

### Image Size Dialog Features
- **Width/Height Spinboxes**: Input custom dimensions (1-10,000 pixels)
- **Maintain Aspect Ratio**: Checkbox (enabled by default)
  - When checked, changing width automatically adjusts height
  - When checked, changing height automatically adjusts width
- **Original Size Display**: Shows the original dimensions of the dropped image
- **OK/Cancel Buttons**: Confirm or abort the import operation

### Canvas Integration
- **Drop Position**: Image is centered at the location where it was dropped
- **Selectable**: Dropped images can be selected using the Selection tool
- **Movable**: Selected images can be moved around the canvas
- **Copy/Paste**: Works with the clipboard (Ctrl+C, Ctrl+V)
- **Undo/Redo**: Fully integrated with the action system (Ctrl+Z, Ctrl+Y)
- **Multiple Images**: Multiple files can be dropped at once

## Usage Instructions

### How to Use Drag-and-Drop

1. **Launch the Application**
   ```bash
   ./FullScreen-Pencil-Draw
   ```

2. **Prepare an Image**
   - Find an image file in your file system (PNG, JPG, BMP, or GIF)
   - Can be from your desktop, file manager, or any folder

3. **Drag the File**
   - Click and hold on the image file
   - Drag it over the FullScreen Pencil Draw window
   - The application will accept the drag operation

4. **Drop the File**
   - Release the mouse button while over the canvas
   - A dialog will immediately appear

5. **Specify Dimensions**
   - Review the original size displayed at the top
   - Enter desired width in the Width field
   - Enter desired height in the Height field
   - Use "Maintain aspect ratio" to preserve proportions
   - Click OK to import or Cancel to abort

6. **Interact with the Image**
   - The image appears on canvas at the drop location
   - Click on it with the Selection tool (S) to select
   - Drag to move it to a different position
   - Copy (Ctrl+C), Cut (Ctrl+X), Paste (Ctrl+V)
   - Delete with Delete key or Edit menu
   - Undo/Redo as needed

## Examples

### Example 1: Scale Down Large Image
- Original: 3840x2160 (4K resolution)
- Desired: 800x450
- Action: Enter 800 in width, height auto-adjusts to 450
- Result: Smaller, manageable image on canvas

### Example 2: Scale Up Small Icon
- Original: 64x64 (small icon)
- Desired: 256x256
- Action: Enter 256 in width, height auto-adjusts to 256
- Result: Larger image suitable for editing

### Example 3: Custom Aspect Ratio
- Original: 800x600
- Desired: 800x400 (wider)
- Action: Uncheck "Maintain aspect ratio", enter 800x400
- Result: Stretched image with custom proportions

## Technical Implementation Details

### Memory Management
- Uses Qt's smart pointers and parent-child ownership
- Properly cleans up dialog on scope exit
- Images added to scene's item list for automatic cleanup

### Performance Considerations
- Uses `Qt::SmoothTransformation` for high-quality scaling
- Scaling happens before adding to scene (not in real-time)
- Efficient for large images

### Integration Points
- Works with existing Canvas class architecture
- Follows Qt's event system patterns
- Integrates with existing Action/Undo system
- Compatible with all existing tools and features

## Testing

### Manual Testing Steps
1. Build the application
2. Run `./FullScreen-Pencil-Draw`
3. Drag various image formats onto canvas
4. Test different dimension specifications
5. Test maintain aspect ratio checkbox
6. Test with very small images (< 100x100)
7. Test with very large images (> 4000x4000)
8. Test undo/redo functionality
9. Test copy/paste of dropped images
10. Test selection and movement

### Build Verification
```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -- -j$(nproc)
```

Should compile without errors. Only acceptable output is warnings (none expected).

## Future Enhancements (Not Implemented)
- Drag-and-drop from web browsers (URLs)
- Preview thumbnail in size dialog
- Recent sizes dropdown
- Batch processing of multiple files
- Additional file format support (SVG, WebP)
- Live preview while adjusting size
- Preset dimension buttons (50%, 100%, 200%)
