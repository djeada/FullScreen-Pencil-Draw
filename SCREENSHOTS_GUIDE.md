# Drag-and-Drop Feature Screenshots Guide

Since this is a headless environment, screenshots cannot be taken automatically. 
Below are descriptions of what each screenshot would show for manual testing:

## Screenshot 1: Initial State
**Filename:** `screenshot1-initial.png`
**Description:** 
- FullScreen-Pencil-Draw application open
- Empty canvas with black background
- Toolbar visible on the left side
- Status bar at bottom showing keyboard shortcuts

## Screenshot 2: Dragging File
**Filename:** `screenshot2-dragging.png`
**Description:**
- File manager window visible beside the application
- User dragging an image file (e.g., test.png)
- Cursor shows drag operation in progress
- Canvas ready to accept the drop

## Screenshot 3: Image Size Dialog
**Filename:** `screenshot3-dialog.png`
**Description:**
- "Specify Image Dimensions" dialog window visible
- Shows "Original size: 800 x 600 px" label
- Width spinbox showing "800 px"
- Height spinbox showing "600 px"
- "Maintain aspect ratio" checkbox is checked
- OK and Cancel buttons at bottom

## Screenshot 4: Adjusting Dimensions
**Filename:** `screenshot4-adjusting.png`
**Description:**
- Same dialog with modified values
- Width changed to "400 px"
- Height automatically adjusted to "300 px" (maintaining aspect ratio)
- Demonstrates the aspect ratio feature working

## Screenshot 5: Image Dropped on Canvas
**Filename:** `screenshot5-dropped.png`
**Description:**
- Image now visible on the canvas
- Image scaled to 400x300 pixels
- Positioned at the location where it was dropped
- Image has selection handles visible
- Can be moved around the canvas

## Screenshot 6: Multiple Images
**Filename:** `screenshot6-multiple.png`
**Description:**
- Multiple images dropped onto canvas
- Each at different sizes
- Some selected, some not
- Demonstrates that feature works with multiple files

## Screenshot 7: Error Message - Unsupported File
**Filename:** `screenshot7-error-unsupported.png`
**Description:**
- Error dialog: "Unsupported File"
- Message: "File 'document.txt' is not a supported image format."
- "Supported formats: PNG, JPG, JPEG, BMP, GIF"
- OK button to dismiss

## Screenshot 8: Error Message - Invalid Image
**Filename:** `screenshot8-error-invalid.png`
**Description:**
- Error dialog: "Invalid Image"
- Message: "Failed to load image from 'corrupted.png'."
- "The file may be corrupted or not a valid image."
- OK button to dismiss

## Screenshot 9: Undo/Redo Testing
**Filename:** `screenshot9-undo.png`
**Description:**
- Image on canvas
- After pressing Ctrl+Z (undo)
- Image removed from canvas
- Demonstrates undo integration

## Screenshot 10: Copy/Paste Testing
**Filename:** `screenshot10-paste.png`
**Description:**
- Original dropped image selected
- After Ctrl+C (copy) and Ctrl+V (paste)
- Duplicate image visible, offset by 20x20 pixels
- Both images selectable independently

## How to Take These Screenshots

1. **Build and Run:**
   ```bash
   cd build
   ./FullScreen-Pencil-Draw
   ```

2. **For each screenshot:**
   - Perform the action described
   - Press Print Screen or use a screenshot tool
   - Save with the suggested filename
   - Add to repository or documentation

3. **Recommended Tools:**
   - Linux: `gnome-screenshot`, `flameshot`, or `scrot`
   - macOS: Cmd+Shift+4
   - Windows: Snipping Tool or Win+Shift+S

4. **Screenshot Location:**
   Suggested directory: `/docs/screenshots/` or `/images/`

## Video Demonstration (Optional)

Consider recording a video showing:
1. Launching the application
2. Dragging a file from file manager
3. Adjusting dimensions in dialog
4. Toggling aspect ratio checkbox
5. Confirming and seeing image on canvas
6. Moving and selecting the image
7. Testing undo/redo
8. Testing copy/paste
9. Trying unsupported file type

Tools: OBS Studio, SimpleScreenRecorder, QuickTime (macOS)
