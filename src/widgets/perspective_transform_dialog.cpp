/**
 * @file perspective_transform_dialog.cpp
 * @brief Implementation of the PerspectiveTransformDialog.
 */
#include "perspective_transform_dialog.h"
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
// PerspectivePreviewWidget
// ---------------------------------------------------------------------------

PerspectivePreviewWidget::PerspectivePreviewWidget(QWidget *parent)
    : QWidget(parent), dragIndex_(-1) {
  setMinimumSize(200, 200);
  setMouseTracking(true);
  reset();
}

void PerspectivePreviewWidget::reset() {
  corners_[0] = QPointF(0.0, 0.0); // top-left
  corners_[1] = QPointF(1.0, 0.0); // top-right
  corners_[2] = QPointF(1.0, 1.0); // bottom-right
  corners_[3] = QPointF(0.0, 1.0); // bottom-left
  update();
  emit cornersChanged();
}

void PerspectivePreviewWidget::setCorner(int index, const QPointF &pos) {
  if (index < 0 || index > 3)
    return;
  corners_[index] = pos;
  update();
}

QRectF PerspectivePreviewWidget::previewRect() const {
  return QRectF(MARGIN, MARGIN, width() - 2 * MARGIN, height() - 2 * MARGIN);
}

QPointF PerspectivePreviewWidget::cornerToWidget(int index) const {
  QRectF r = previewRect();
  return QPointF(r.left() + corners_[index].x() * r.width(),
                 r.top() + corners_[index].y() * r.height());
}

int PerspectivePreviewWidget::cornerIndexAt(const QPointF &widgetPos) const {
  for (int i = 0; i < 4; ++i) {
    QPointF cp = cornerToWidget(i);
    if (QLineF(cp, widgetPos).length() <= HANDLE_RADIUS + 4)
      return i;
  }
  return -1;
}

void PerspectivePreviewWidget::paintEvent(QPaintEvent * /*event*/) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  QRectF r = previewRect();

  // Draw original rectangle as dashed
  QPen dashPen(Qt::gray, 1.0, Qt::DashLine);
  p.setPen(dashPen);
  p.setBrush(Qt::NoBrush);
  p.drawRect(r);

  // Draw perspective quad
  QPolygonF quad;
  for (int i = 0; i < 4; ++i)
    quad << cornerToWidget(i);

  QPen solidPen(QColor(0, 120, 215), 2.0);
  p.setPen(solidPen);
  p.setBrush(QColor(0, 120, 215, 30));
  p.drawPolygon(quad);

  // Draw corner handles
  for (int i = 0; i < 4; ++i) {
    QPointF cp = cornerToWidget(i);
    p.setPen(QPen(QColor(0, 120, 215), 1.5));
    p.setBrush(Qt::white);
    p.drawEllipse(cp, HANDLE_RADIUS, HANDLE_RADIUS);
  }

  // Labels
  p.setPen(Qt::black);
  QFont f = font();
  f.setPointSize(8);
  p.setFont(f);
  const char *labels[] = {"TL", "TR", "BR", "BL"};
  for (int i = 0; i < 4; ++i) {
    QPointF cp = cornerToWidget(i);
    p.drawText(cp + QPointF(-6, -10), labels[i]);
  }
}

void PerspectivePreviewWidget::mousePressEvent(QMouseEvent *event) {
  dragIndex_ = cornerIndexAt(event->pos());
  if (dragIndex_ >= 0)
    setCursor(Qt::ClosedHandCursor);
}

void PerspectivePreviewWidget::mouseMoveEvent(QMouseEvent *event) {
  if (dragIndex_ < 0) {
    int hover = cornerIndexAt(event->pos());
    setCursor(hover >= 0 ? Qt::OpenHandCursor : Qt::ArrowCursor);
    return;
  }

  QRectF r = previewRect();
  if (r.width() <= 0 || r.height() <= 0)
    return;

  QPointF pos = event->pos();
  double nx = qBound(0.0, (pos.x() - r.left()) / r.width(), 1.0);
  double ny = qBound(0.0, (pos.y() - r.top()) / r.height(), 1.0);
  corners_[dragIndex_] = QPointF(nx, ny);
  update();
  emit cornersChanged();
}

void PerspectivePreviewWidget::mouseReleaseEvent(QMouseEvent * /*event*/) {
  dragIndex_ = -1;
  setCursor(Qt::ArrowCursor);
}

// ---------------------------------------------------------------------------
// PerspectiveTransformDialog
// ---------------------------------------------------------------------------

PerspectiveTransformDialog::PerspectiveTransformDialog(QWidget *parent)
    : QDialog(parent), updatingFromPreview_(false), updatingFromSpinBox_(false) {
  setWindowTitle("Perspective Transform");
  setModal(true);
  setMinimumWidth(460);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(16, 16, 16, 16);
  mainLayout->setSpacing(12);

  // Preview
  preview_ = new PerspectivePreviewWidget(this);
  preview_->setFixedSize(260, 260);
  mainLayout->addWidget(preview_, 0, Qt::AlignCenter);

  // Spin-boxes organised as two rows (top-left / top-right, etc.)
  auto createPair = [&](const QString &label, QDoubleSpinBox *&sx,
                        QDoubleSpinBox *&sy) -> QGroupBox * {
    QGroupBox *box = new QGroupBox(label, this);
    QFormLayout *fl = new QFormLayout(box);
    fl->setSpacing(6);
    sx = createSpinBox();
    sy = createSpinBox();
    fl->addRow("X %:", sx);
    fl->addRow("Y %:", sy);
    return box;
  };

  QHBoxLayout *topRow = new QHBoxLayout();
  topRow->addWidget(createPair("Top Left", tlX_, tlY_));
  topRow->addWidget(createPair("Top Right", trX_, trY_));
  mainLayout->addLayout(topRow);

  QHBoxLayout *bottomRow = new QHBoxLayout();
  bottomRow->addWidget(createPair("Bottom Left", blX_, blY_));
  bottomRow->addWidget(createPair("Bottom Right", brX_, brY_));
  mainLayout->addLayout(bottomRow);

  // Reset button + dialog buttons
  QHBoxLayout *btnLayout = new QHBoxLayout();
  QPushButton *resetBtn = new QPushButton("Reset", this);
  resetBtn->setMinimumHeight(36);
  btnLayout->addWidget(resetBtn);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  buttonBox->button(QDialogButtonBox::Ok)->setMinimumHeight(36);
  buttonBox->button(QDialogButtonBox::Cancel)->setMinimumHeight(36);
  btnLayout->addWidget(buttonBox);
  mainLayout->addLayout(btnLayout);

  // Connections
  connect(preview_, &PerspectivePreviewWidget::cornersChanged, this,
          &PerspectiveTransformDialog::onCornersChanged);

  auto connectSpin = [&](QDoubleSpinBox *sb) {
    connect(sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &PerspectiveTransformDialog::onSpinBoxChanged);
  };
  connectSpin(tlX_);
  connectSpin(tlY_);
  connectSpin(trX_);
  connectSpin(trY_);
  connectSpin(blX_);
  connectSpin(blY_);
  connectSpin(brX_);
  connectSpin(brY_);

  connect(resetBtn, &QPushButton::clicked, this,
          &PerspectiveTransformDialog::onReset);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  // Initial sync
  syncSpinBoxesFromPreview();
}

QDoubleSpinBox *PerspectiveTransformDialog::createSpinBox() {
  auto *sb = new QDoubleSpinBox(this);
  sb->setRange(-50.0, 50.0);
  sb->setValue(0.0);
  sb->setSuffix(" %");
  sb->setDecimals(1);
  sb->setSingleStep(1.0);
  sb->setMinimumHeight(28);
  return sb;
}

void PerspectiveTransformDialog::onCornersChanged() {
  if (updatingFromSpinBox_)
    return;
  syncSpinBoxesFromPreview();
}

void PerspectiveTransformDialog::onSpinBoxChanged() {
  if (updatingFromPreview_)
    return;
  syncPreviewFromSpinBoxes();
}

void PerspectiveTransformDialog::onReset() {
  preview_->reset();
  syncSpinBoxesFromPreview();
}

void PerspectiveTransformDialog::syncSpinBoxesFromPreview() {
  updatingFromPreview_ = true;

  // Corner offsets as percentage of size. Default positions are
  // TL(0,0), TR(1,0), BR(1,1), BL(0,1).
  auto toPercent = [](double actual, double def) {
    return (actual - def) * 100.0;
  };

  tlX_->setValue(toPercent(preview_->topLeft().x(), 0.0));
  tlY_->setValue(toPercent(preview_->topLeft().y(), 0.0));
  trX_->setValue(toPercent(preview_->topRight().x(), 1.0));
  trY_->setValue(toPercent(preview_->topRight().y(), 0.0));
  brX_->setValue(toPercent(preview_->bottomRight().x(), 1.0));
  brY_->setValue(toPercent(preview_->bottomRight().y(), 1.0));
  blX_->setValue(toPercent(preview_->bottomLeft().x(), 0.0));
  blY_->setValue(toPercent(preview_->bottomLeft().y(), 1.0));

  updatingFromPreview_ = false;
}

void PerspectiveTransformDialog::syncPreviewFromSpinBoxes() {
  updatingFromSpinBox_ = true;

  auto fromPercent = [](double pct, double def) { return def + pct / 100.0; };

  preview_->setCorner(
      0, QPointF(fromPercent(tlX_->value(), 0.0),
                 fromPercent(tlY_->value(), 0.0)));
  preview_->setCorner(
      1, QPointF(fromPercent(trX_->value(), 1.0),
                 fromPercent(trY_->value(), 0.0)));
  preview_->setCorner(
      2, QPointF(fromPercent(brX_->value(), 1.0),
                 fromPercent(brY_->value(), 1.0)));
  preview_->setCorner(
      3, QPointF(fromPercent(blX_->value(), 0.0),
                 fromPercent(blY_->value(), 1.0)));

  preview_->update();
  updatingFromSpinBox_ = false;
}

QTransform
PerspectiveTransformDialog::perspectiveTransform(const QRectF &rect) const {
  if (rect.isEmpty())
    return QTransform();

  // Source corners of the bounding rect
  QPolygonF src;
  src << rect.topLeft() << rect.topRight() << rect.bottomRight()
      << rect.bottomLeft();

  // Destination corners based on preview offsets
  QPolygonF dst;
  dst << QPointF(rect.left() + preview_->topLeft().x() * rect.width(),
                 rect.top() + preview_->topLeft().y() * rect.height());
  dst << QPointF(rect.left() + preview_->topRight().x() * rect.width(),
                 rect.top() + preview_->topRight().y() * rect.height());
  dst << QPointF(rect.left() + preview_->bottomRight().x() * rect.width(),
                 rect.top() + preview_->bottomRight().y() * rect.height());
  dst << QPointF(rect.left() + preview_->bottomLeft().x() * rect.width(),
                 rect.top() + preview_->bottomLeft().y() * rect.height());

  // Qt provides QTransform::quadToQuad which computes the projective
  // (perspective) matrix mapping one quadrilateral to another.
  QTransform t;
  if (!QTransform::quadToQuad(src, dst, t)) {
    return QTransform(); // identity if degenerate
  }
  return t;
}
