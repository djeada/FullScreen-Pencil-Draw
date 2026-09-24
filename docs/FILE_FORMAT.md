# Native project format (`.fspd`)

The `.fspd` file is the only format that keeps a document fully editable.
It is a UTF-8 JSON document written by `ProjectSerializer`
(`src/core/project_serializer.*`).

## Guarantees

- **Lossless round trip.** Save → close → reopen gives back every layer
  (id, name, type, order, visibility, lock, opacity, blend mode), every item
  (stable id, stacking order, layer membership, position, z, visibility,
  opacity, transform, rotation/scale/origin, lock) and each item's own
  editable data. `tests/test_document_integrity.cpp` checks this
  structurally and by comparing renders before and after.
- **No silent loss.** An item type without a representation below is never
  dropped or flattened without the user knowing: the save reports it
  (`ProjectSaveError::UnsupportedContent`) and the app asks whether to save
  those items as images or cancel. Only crash-recovery snapshots flatten such
  items automatically, since they must lose as little as possible.
- **Atomic, verified writes.** The document is serialized and validated in
  memory first, then written with `QSaveFile`; the target is replaced only
  after every byte has been written and committed, and the file is then read
  back and compared. Any failure (missing folder, permissions, disk full,
  commit error, verification mismatch) leaves the previous file untouched,
  reports the path and reason, and keeps the document marked as modified.
- **Safe loading.** Every item is rebuilt before the open document is
  touched, so a damaged file cannot leave a half-replaced document behind.
  Raster data is validated (PNG decodes, tile size, coordinate range, tile
  count) before any allocation.

## Versions

| `formatVersion` | Written by | Read by current version |
|---|---|---|
| 1 | older releases | yes; missing fields take their defaults |
| 2 | current | yes |

Files with a newer version are rejected with a message naming the version.
Version 2 only adds fields (layer `id`; item `id`; `brushStroke` and
`raster` items; `backgroundImage`; the full font string; pen dash pattern,
miter limit and cosmetic flag; ellipse spans; path fill rule; image tint;
text width; rotation/scale/origin), so v1 content needs no migration.

## Layout

```json
{
  "formatVersion": 2,
  "application": "FullScreenPencilDraw",
  "canvas": { "x": 0, "y": 0, "width": 1920, "height": 1080,
              "backgroundColor": "#ff000000" },
  "backgroundImage": { "data": "<PNG base64>", "dpr": 1, "x": 0, "y": 0,
                       "z": -1000, "visible": true },
  "activeLayer": 0,
  "layers": [
    { "id": "<uuid>", "name": "Background", "type": 0, "blendMode": 0,
      "visible": true, "locked": false, "opacity": 1,
      "items": [ { "id": "<uuid>", "type": "path", "...": "..." } ] }
  ]
}
```

`backgroundImage` is the image opened with File → Open, which sits beneath
every layer. Layer `type` is 0 = vector, 1 = raster, 2 = mixed. `blendMode`
follows `Layer::BlendMode` (0 = Normal, 1 = Multiply, 2 = Screen, …).
Items are listed bottom to top.

## Item types

Every item also carries the common fields `id x y z visible opacity
transform` and, when set, `rotation scale originX originY locked`.

| `type` | Editable as | Payload |
|---|---|---|
| `path` | vector path | `pen`, `brush`, `fillRule`, `pathElements` |
| `rect`, `ellipse` | vector shape | `pen`, `brush`, `rx ry rw rh` (+ `startAngle spanAngle`) |
| `line` | vector line | `pen`, `x1 y1 x2 y2` |
| `polygon` | vector polygon | `pen`, `brush`, `fillRule`, `points` |
| `group` | group (arrows, user groups) | `children` (group-local items) |
| `text` | rich text | `html`, `defaultColor`, `font`, `textWidth` |
| `latexText` | text with LaTeX | `text`, `textColor`, `font` |
| `textOnPath` | text along a path | `text`, `textColor`, `font`, `pathElements` |
| `mermaid` | Mermaid diagram | `code`, `theme` |
| `element` | electronics / architecture element | `elementId`, `key` |
| `wire` | wire between elements | `srcKey srcPin dstKey dstPin pen` |
| `pixmap` | image | `data` (PNG), `dpr`, `offsetX offsetY`, `transformationMode`, `tint` |
| `brushStroke` | custom brush-tip stroke | `tip` (shape, angle, spacing, image), `size`, `color`, `strokeOpacity`, `points`, `image` (PNG), `imageX imageY` |
| `raster` | raster layer pixels | `tileSize` (256), `tiles`: `[{tx, ty, png}]` |

`pen`: `color width style capStyle joinStyle miterLimit cosmetic`
(+ `dashPattern dashOffset`). `brush`: `color style` (+ `gradient`:
linear/radial/conical with stops, spread and coordinate mode). `font`:
`QFont::toString()` (the older `fontFamily fontSize fontBold fontItalic`
fields are still written for older readers).

A `pixmap` with a `rasterizedFrom` field was flattened from an unsupported
item with the user's consent (or by a recovery snapshot).

## Raster layers

Raster pixels are stored as 256×256 premultiplied-ARGB tiles in the layer's
own pixel coordinates. Only tiles that contain pixels are written, and each
one is a PNG. Loading rejects a document that has malformed tiles, duplicate
tiles, coordinates beyond ±2²⁰ px or more than 16 384 tiles.

## Crash-recovery snapshots

Auto-save writes this same format to
`<app data>/recovery/<document-id>.fspd`, with `<document-id>.json`
metadata (original path, display name, time, layer and object counts) and a
`<document-id>.lock` held while the document is open. See
`src/core/recovery_store.h`.
