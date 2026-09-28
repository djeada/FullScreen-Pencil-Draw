# Erasers

FullScreen Pencil Draw edits vector objects and raster pixels, so it has two
separate erasers. They are different tools, and neither replaces the other.

## Object Eraser (`E`)

Deletes **whole objects**: paths, shapes, text, images, brush strokes, groups
and diagram elements.

- An object is erased only if the eraser touches what you actually see: the
  stroked outline or filled area of a vector object, or the non-transparent
  pixels of an image or brush stroke. Touching the empty part of an object's
  bounding box does nothing.
- Objects on hidden or locked layers, locked objects and hidden objects are
  never erased.
- Raster-layer pixels are not objects, so the Object Eraser ignores them.
  Use the Pixel Eraser for pixels.
- Wires attached to an erased element go with it.
- One drag is one undo step. Undo puts every object back in its layer at its
  original stacking position.

## Pixel Eraser (`Shift+E`)

Removes **pixels** from raster content on the **active layer** only:

- raster (pixel) layers (Layer → New Raster Layer),
- images,
- custom brush-tip strokes.

It never deletes the image, stroke or layer and never changes vector
objects. If the active layer holds only vector objects, the status bar
explains this instead of deleting or rasterizing anything. A hidden or
locked active layer is left alone.

- **Size**: the eraser size (`[` / `]`).
- **Strength**: Tools → Pixel Eraser Strength (how much alpha one pass
  removes).
- **Hardness**: Tools → Pixel Eraser Hardness (100 = hard edge, lower =
  soft falloff).
- **Selection clipping**: with a colour selection active on an image, only
  selected pixels are erased.
- Fast strokes are erased along the whole path, without gaps.
- One drag is one undo step. On raster layers the undo step stores only the
  256×256 tiles the drag touched, not the whole layer.

The result survives save → reopen and appears in every export: SVG and PDF
embed the erased image, and bitmap exports show the composite.

## Future: Vector Stroke Eraser (not implemented)

A third mode could cut vector paths where the eraser crosses them, splitting
one path into several instead of deleting it. It would be a separate tool
from the two above, because it changes vector geometry, while the Object
Eraser removes objects and the Pixel Eraser removes pixels. It needs its own
undo semantics (one path becomes several objects) and hit-testing against
stroke outlines, so it is tracked separately and does not block the
two-eraser design.
