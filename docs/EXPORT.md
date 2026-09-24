# Export formats

FullScreen Pencil Draw keeps vector objects and raster pixels side by side.
What survives outside the app depends on what the target format can hold.
`DocumentExporter` (`src/core/document_exporter.*`) implements the rules
below for every export path: File → Export, File → Export to PDF, and the
selection exports in the context menu. The app lists anything a format could
not keep after the export finishes.

## Capability matrix

| Format | Kind | Vector objects | Raster content | Transparency | Layer blend modes |
|---|---|---|---|---|---|
| `.fspd` | editable native | kept, editable | kept, editable | kept | kept |
| SVG | hybrid vector + raster | vector elements | embedded PNG images | kept | exported as Normal, **reported** |
| PDF | hybrid vector + raster | vector | embedded images at their own resolution | page filled with the canvas colour | exported as Normal, **reported** |
| PNG, WebP, TIFF | flattened | rendered | rendered | kept (transparent canvas stays transparent) | applied |
| JPEG, BMP | flattened | rendered | rendered | none: flattened onto white, **reported** | applied |

- **Editable native save** is the only format you can reopen with layers
  and objects intact. See [FILE_FORMAT.md](FILE_FORMAT.md).
- **Hybrid exports** (SVG, PDF) draw paths, shapes, lines and plain text as
  vector geometry, and embed images, brush strokes and raster-layer tiles as
  images. Vector content is never turned into pixels to imitate an effect.
- **Flattened exports** are a deliberate composite made by the same
  code as the on-screen layer compositing, so they match the canvas.

## Format-specific rules

- **LaTeX text and Mermaid diagrams** are drawn from rendered images inside
  the app, so SVG and PDF embed them as images. The export reports how many
  there are.
- **Blend modes in SVG/PDF.** Qt's SVG and PDF writers cannot express layer
  blend modes, so those layers are drawn as Normal and the export lists them
  by name. Use PNG, WebP or TIFF when the blended look matters.
- **PDF page.** The drawing is scaled to fit an A4 page with 10 mm margins,
  centred, on a page filled with the canvas colour.
- **Annotated PDF export** (PDF viewer) keeps every page at its own size,
  draws the original page as a rendered background image, and puts that
  page's own annotations on top of it in the same position.
- **Hidden layers and hidden objects** are never exported; exports follow
  the same layer order, visibility, opacity and blend modes as the canvas.
- **Exports never mark the document as saved.** Only a native save does.

## Importing SVG

Opening an SVG file currently rasterizes it into an image (through
`QSvgRenderer`). Supporting SVG export does not mean SVG import is
editable; editable SVG import would be a separate feature.

## Tests

`tests/test_document_integrity.cpp` exports one mixed document (vector
paths, shapes, text, a raster layer with erased pixels, an image, a brush
stroke and blended layers) and checks that:

- the SVG contains vector paths and embedded images, and reports the blend
  modes;
- the PDF embeds image data;
- the PNG export matches the composite, with blending applied and erased
  pixels gone;
- JPEG flattens transparency onto white and reports it, while PNG keeps it.
