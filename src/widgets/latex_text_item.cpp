/**
 * @file latex_text_item.cpp
 * @brief Implementation of LaTeX-enabled text graphics item with inline
 * editing.
 */
#include "latex_text_item.h"
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QFocusEvent>
#include <QFontDatabase>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QPointer>
#include <QRegularExpression>
#include <QStyleOptionGraphicsItem>
#include <QTextDocument>
#include <cmath>
#include <functional>

#ifdef HAVE_QT_WEBENGINE
#include "../core/katex_renderer.h"
#endif

// Unicode math symbols for LaTeX rendering
namespace LatexSymbols {
// Greek letters (lowercase and uppercase)
const QMap<QString, QString> greekLetters = {
    {"alpha", "α"},   {"beta", "β"},       {"gamma", "γ"},    {"delta", "δ"},
    {"epsilon", "ε"}, {"varepsilon", "ɛ"}, {"zeta", "ζ"},     {"eta", "η"},
    {"theta", "θ"},   {"vartheta", "ϑ"},   {"iota", "ι"},     {"kappa", "κ"},
    {"lambda", "λ"},  {"mu", "μ"},         {"nu", "ν"},       {"xi", "ξ"},
    {"omicron", "ο"}, {"pi", "π"},         {"varpi", "ϖ"},    {"rho", "ρ"},
    {"varrho", "ϱ"},  {"sigma", "σ"},      {"varsigma", "ς"}, {"tau", "τ"},
    {"upsilon", "υ"}, {"phi", "φ"},        {"varphi", "ϕ"},   {"chi", "χ"},
    {"psi", "ψ"},     {"omega", "ω"},      {"Alpha", "Α"},    {"Beta", "Β"},
    {"Gamma", "Γ"},   {"Delta", "Δ"},      {"Epsilon", "Ε"},  {"Zeta", "Ζ"},
    {"Eta", "Η"},     {"Theta", "Θ"},      {"Iota", "Ι"},     {"Kappa", "Κ"},
    {"Lambda", "Λ"},  {"Mu", "Μ"},         {"Nu", "Ν"},       {"Xi", "Ξ"},
    {"Omicron", "Ο"}, {"Pi", "Π"},         {"Rho", "Ρ"},      {"Sigma", "Σ"},
    {"Tau", "Τ"},     {"Upsilon", "Υ"},    {"Phi", "Φ"},      {"Chi", "Χ"},
    {"Psi", "Ψ"},     {"Omega", "Ω"}};

// Math operators and symbols (extended)
const QMap<QString, QString> mathSymbols = {
    // Basic operators
    {"cdot", "·"},
    {"times", "×"},
    {"div", "÷"},
    {"pm", "±"},
    {"mp", "∓"},
    {"ast", "∗"},
    {"star", "⋆"},
    {"circ", "∘"},
    {"bullet", "•"},
    {"oplus", "⊕"},
    {"ominus", "⊖"},
    {"otimes", "⊗"},
    {"oslash", "⊘"},
    {"odot", "⊙"},
    // Relations
    {"leq", "≤"},
    {"geq", "≥"},
    {"neq", "≠"},
    {"approx", "≈"},
    {"equiv", "≡"},
    {"sim", "∼"},
    {"simeq", "≃"},
    {"cong", "≅"},
    {"propto", "∝"},
    {"ll", "≪"},
    {"gg", "≫"},
    {"prec", "≺"},
    {"succ", "≻"},
    {"preceq", "⪯"},
    {"succeq", "⪰"},
    {"perp", "⊥"},
    {"parallel", "∥"},
    {"asymp", "≍"},
    {"doteq", "≐"},
    {"models", "⊨"},
    {"vdash", "⊢"},
    {"dashv", "⊣"},
    // Set theory
    {"in", "∈"},
    {"notin", "∉"},
    {"ni", "∋"},
    {"subset", "⊂"},
    {"supset", "⊃"},
    {"subseteq", "⊆"},
    {"supseteq", "⊇"},
    {"nsubseteq", "⊈"},
    {"nsupseteq", "⊉"},
    {"cup", "∪"},
    {"cap", "∩"},
    {"setminus", "∖"},
    {"emptyset", "∅"},
    {"varnothing", "∅"},
    // Logic
    {"forall", "∀"},
    {"exists", "∃"},
    {"nexists", "∄"},
    {"land", "∧"},
    {"lor", "∨"},
    {"lnot", "¬"},
    {"neg", "¬"},
    {"therefore", "∴"},
    {"because", "∵"},
    {"implies", "⟹"},
    {"iff", "⟺"},
    {"top", "⊤"},
    {"bot", "⊥"},
    // Arrows
    {"rightarrow", "→"},
    {"leftarrow", "←"},
    {"leftrightarrow", "↔"},
    {"Rightarrow", "⇒"},
    {"Leftarrow", "⇐"},
    {"Leftrightarrow", "⇔"},
    {"longrightarrow", "⟶"},
    {"longleftarrow", "⟵"},
    {"Longrightarrow", "⟹"},
    {"Longleftarrow", "⟸"},
    {"mapsto", "↦"},
    {"longmapsto", "⟼"},
    {"uparrow", "↑"},
    {"downarrow", "↓"},
    {"updownarrow", "↕"},
    {"Uparrow", "⇑"},
    {"Downarrow", "⇓"},
    {"Updownarrow", "⇕"},
    {"nearrow", "↗"},
    {"searrow", "↘"},
    {"nwarrow", "↖"},
    {"swarrow", "↙"},
    {"hookrightarrow", "↪"},
    {"hookleftarrow", "↩"},
    // Calculus and analysis
    {"infty", "∞"},
    {"partial", "∂"},
    {"nabla", "∇"},
    {"sum", "∑"},
    {"prod", "∏"},
    {"coprod", "∐"},
    {"int", "∫"},
    {"iint", "∬"},
    {"iiint", "∭"},
    {"oint", "∮"},
    {"oiint", "∯"},
    {"sqrt", "√"},
    {"cbrt", "∛"},
    {"fourthroot", "∜"},
    {"lim", "lim"},
    {"limsup", "lim sup"},
    {"liminf", "lim inf"},
    {"max", "max"},
    {"min", "min"},
    {"sup", "sup"},
    {"inf", "inf"},
    {"arg", "arg"},
    {"det", "det"},
    {"dim", "dim"},
    {"ker", "ker"},
    {"hom", "hom"},
    {"deg", "deg"},
    {"exp", "exp"},
    {"log", "log"},
    {"ln", "ln"},
    {"lg", "lg"},
    {"sin", "sin"},
    {"cos", "cos"},
    {"tan", "tan"},
    {"cot", "cot"},
    {"sec", "sec"},
    {"csc", "csc"},
    {"arcsin", "arcsin"},
    {"arccos", "arccos"},
    {"arctan", "arctan"},
    {"sinh", "sinh"},
    {"cosh", "cosh"},
    {"tanh", "tanh"},
    {"coth", "coth"},
    // Geometry
    {"angle", "∠"},
    {"measuredangle", "∡"},
    {"sphericalangle", "∢"},
    {"triangle", "△"},
    {"square", "□"},
    {"diamond", "◇"},
    {"degree", "°"},
    {"perp", "⊥"},
    {"parallel", "∥"},
    // Miscellaneous
    {"ldots", "…"},
    {"cdots", "⋯"},
    {"vdots", "⋮"},
    {"ddots", "⋱"},
    {"prime", "′"},
    {"dprime", "″"},
    {"hbar", "ℏ"},
    {"ell", "ℓ"},
    {"wp", "℘"},
    {"Re", "ℜ"},
    {"Im", "ℑ"},
    {"aleph", "ℵ"},
    {"beth", "ℶ"},
    {"gimel", "ℷ"},
    {"daleth", "ℸ"},
    {"complement", "∁"},
    {"backslash", "\\"},
    {"surd", "√"},
    {"dagger", "†"},
    {"ddagger", "‡"},
    {"S", "§"},
    {"P", "¶"},
    {"copyright", "©"},
    {"registered", "®"},
    {"trademark", "™"},
    {"pounds", "£"},
    {"euro", "€"},
    {"yen", "¥"},
    {"cent", "¢"},
    // Brackets and delimiters
    {"langle", "⟨"},
    {"rangle", "⟩"},
    {"lfloor", "⌊"},
    {"rfloor", "⌋"},
    {"lceil", "⌈"},
    {"rceil", "⌉"},
    {"lbrace", "{"},
    {"rbrace", "}"},
    {"lbrack", "["},
    {"rbrack", "]"},
    {"vert", "|"},
    {"Vert", "‖"},
    // Special characters
    {"quad", "  "},
    {"qquad", "    "},
    {"enspace", " "},
    {"thinspace", " "},
    {"negthickspace", ""},
    {"negthinspace", ""},
    {"colon", ":"},
    {"dots", "…"},
    // Text formatting
    {"textbf", ""},
    {"textit", ""},
    {"textrm", ""},
    {"mathrm", ""},
    {"mathbf", ""},
    {"mathit", ""},
    {"mathcal", ""},
    {"mathbb", ""},
    {"mathfrak", ""}};

// Superscript characters (extended)
const QMap<QChar, QString> superscripts = {
    {'0', "⁰"}, {'1', "¹"}, {'2', "²"}, {'3', "³"}, {'4', "⁴"}, {'5', "⁵"},
    {'6', "⁶"}, {'7', "⁷"}, {'8', "⁸"}, {'9', "⁹"}, {'+', "⁺"}, {'-', "⁻"},
    {'=', "⁼"}, {'(', "⁽"}, {')', "⁾"}, {'a', "ᵃ"}, {'b', "ᵇ"}, {'c', "ᶜ"},
    {'d', "ᵈ"}, {'e', "ᵉ"}, {'f', "ᶠ"}, {'g', "ᵍ"}, {'h', "ʰ"}, {'i', "ⁱ"},
    {'j', "ʲ"}, {'k', "ᵏ"}, {'l', "ˡ"}, {'m', "ᵐ"}, {'n', "ⁿ"}, {'o', "ᵒ"},
    {'p', "ᵖ"}, {'r', "ʳ"}, {'s', "ˢ"}, {'t', "ᵗ"}, {'u', "ᵘ"}, {'v', "ᵛ"},
    {'w', "ʷ"}, {'x', "ˣ"}, {'y', "ʸ"}, {'z', "ᶻ"}};

// Subscript characters (extended)
const QMap<QChar, QString> subscripts = {
    {'0', "₀"}, {'1', "₁"}, {'2', "₂"}, {'3', "₃"}, {'4', "₄"}, {'5', "₅"},
    {'6', "₆"}, {'7', "₇"}, {'8', "₈"}, {'9', "₉"}, {'+', "₊"}, {'-', "₋"},
    {'=', "₌"}, {'(', "₍"}, {')', "₎"}, {'a', "ₐ"}, {'e', "ₑ"}, {'h', "ₕ"},
    {'i', "ᵢ"}, {'j', "ⱼ"}, {'k', "ₖ"}, {'l', "ₗ"}, {'m', "ₘ"}, {'n', "ₙ"},
    {'o', "ₒ"}, {'p', "ₚ"}, {'r', "ᵣ"}, {'s', "ₛ"}, {'t', "ₜ"}, {'u', "ᵤ"},
    {'v', "ᵥ"}, {'x', "ₓ"}};

// Blackboard bold (double-struck) letters for \mathbb
const QMap<QChar, QString> mathbb = {
    {'A', "𝔸"}, {'B', "𝔹"}, {'C', "ℂ"}, {'D', "𝔻"}, {'E', "𝔼"}, {'F', "𝔽"},
    {'G', "𝔾"}, {'H', "ℍ"}, {'I', "𝕀"}, {'J', "𝕁"}, {'K', "𝕂"}, {'L', "𝕃"},
    {'M', "𝕄"}, {'N', "ℕ"}, {'O', "𝕆"}, {'P', "ℙ"}, {'Q', "ℚ"}, {'R', "ℝ"},
    {'S', "𝕊"}, {'T', "𝕋"}, {'U', "𝕌"}, {'V', "𝕍"}, {'W', "𝕎"}, {'X', "𝕏"},
    {'Y', "𝕐"}, {'Z', "ℤ"}, {'1', "𝟙"}};

// Calligraphic letters for \mathcal
const QMap<QChar, QString> mathcal = {
    {'A', "𝒜"}, {'B', "ℬ"}, {'C', "𝒞"}, {'D', "𝒟"}, {'E', "ℰ"}, {'F', "ℱ"},
    {'G', "𝒢"}, {'H', "ℋ"}, {'I', "ℐ"}, {'J', "𝒥"}, {'K', "𝒦"}, {'L', "ℒ"},
    {'M', "ℳ"}, {'N', "𝒩"}, {'O', "𝒪"}, {'P', "𝒫"}, {'Q', "𝒬"}, {'R', "ℛ"},
    {'S', "𝒮"}, {'T', "𝒯"}, {'U', "𝒰"}, {'V', "𝒱"}, {'W', "𝒲"}, {'X', "𝒳"},
    {'Y', "𝒴"}, {'Z', "𝒵"}};

// Fraktur letters for \mathfrak
const QMap<QChar, QString> mathfrak = {
    {'A', "𝔄"}, {'B', "𝔅"}, {'C', "ℭ"}, {'D', "𝔇"}, {'E', "𝔈"}, {'F', "𝔉"},
    {'G', "𝔊"}, {'H', "ℌ"}, {'I', "ℑ"}, {'J', "𝔍"}, {'K', "𝔎"}, {'L', "𝔏"},
    {'M', "𝔐"}, {'N', "𝔑"}, {'O', "𝔒"}, {'P', "𝔓"}, {'Q', "𝔔"}, {'R', "ℜ"},
    {'S', "𝔖"}, {'T', "𝔗"}, {'U', "𝔘"}, {'V', "𝔙"}, {'W', "𝔚"}, {'X', "𝔛"},
    {'Y', "𝔜"}, {'Z', "ℨ"}};

// Mathematical italic letters for variable styling
const QMap<QChar, QString> mathItalic = {
    {'A', "𝐴"}, {'B', "𝐵"}, {'C', "𝐶"}, {'D', "𝐷"}, {'E', "𝐸"}, {'F', "𝐹"},
    {'G', "𝐺"}, {'H', "𝐻"}, {'I', "𝐼"}, {'J', "𝐽"}, {'K', "𝐾"}, {'L', "𝐿"},
    {'M', "𝑀"}, {'N', "𝑁"}, {'O', "𝑂"}, {'P', "𝑃"}, {'Q', "𝑄"}, {'R', "𝑅"},
    {'S', "𝑆"}, {'T', "𝑇"}, {'U', "𝑈"}, {'V', "𝑉"}, {'W', "𝑊"}, {'X', "𝑋"},
    {'Y', "𝑌"}, {'Z', "𝑍"}, {'a', "𝑎"}, {'b', "𝑏"}, {'c', "𝑐"}, {'d', "𝑑"},
    {'e', "𝑒"}, {'f', "𝑓"}, {'g', "𝑔"}, {'h', "ℎ"}, {'i', "𝑖"}, {'j', "𝑗"},
    {'k', "𝑘"}, {'l', "𝑙"}, {'m', "𝑚"}, {'n', "𝑛"}, {'o', "𝑜"}, {'p', "𝑝"},
    {'q', "𝑞"}, {'r', "𝑟"}, {'s', "𝑠"}, {'t', "𝑡"}, {'u', "𝑢"}, {'v', "𝑣"},
    {'w', "𝑤"}, {'x', "𝑥"}, {'y', "𝑦"}, {'z', "𝑧"}};
} // namespace LatexSymbols

// Convert plain text to HTML while preserving user-authored line breaks.
static QString plainTextToHtmlPreservingNewlines(QString text) {
  text.replace("\r\n", "\n");
  text.replace('\r', '\n');

  QString html = text.toHtmlEscaped();
  html.replace('\n', "<br/>");
  return html;
}

// LatexTextEdit implementation
LatexTextEdit::LatexTextEdit(QWidget *parent) : QTextEdit(parent) {
  setFrameStyle(QFrame::Box);
  setLineWidth(2);
  setStyleSheet(
      "QTextEdit {"
      "  background-color: #1a1a24;"
      "  color: #e0e6f4;"
      "  border: 1px solid #4a5568;"
      "  border-radius: 8px;"
      "  padding: 10px 12px;"
      "  selection-background-color: #3d4f6f;"
      "  selection-color: #ffffff;"
      "  font-family: 'STIX Two Math', 'Cambria Math', 'DejaVu Serif', "
      "'Liberation Serif', serif;"
      "  font-size: 14px;"
      "  line-height: 1.4;"
      "}"
      "QTextEdit:focus {"
      "  border: 1.5px solid #6b8cce;"
      "  background-color: #1e1e2e;"
      "  box-shadow: 0 0 8px rgba(107, 140, 206, 0.3);"
      "}"
      "QScrollBar:vertical {"
      "  background: #252535;"
      "  width: 8px;"
      "  border-radius: 4px;"
      "  margin: 2px;"
      "}"
      "QScrollBar::handle:vertical {"
      "  background: #4a5568;"
      "  border-radius: 4px;"
      "  min-height: 24px;"
      "}"
      "QScrollBar::handle:vertical:hover {"
      "  background: #6b7b8f;"
      "}"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
      "  height: 0px;"
      "}"
      "QScrollBar:horizontal {"
      "  background: #252535;"
      "  height: 8px;"
      "  border-radius: 4px;"
      "  margin: 2px;"
      "}"
      "QScrollBar::handle:horizontal {"
      "  background: #4a5568;"
      "  border-radius: 4px;"
      "  min-width: 24px;"
      "}"
      "QScrollBar::handle:horizontal:hover {"
      "  background: #6b7b8f;"
      "}"
      "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
      "  width: 0px;"
      "}");
  setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setPlaceholderText(
      "Type here... Use $...$ for LaTeX math\n"
      "Examples: $\\alpha + \\beta$, $x^2 + y^2 = r^2$, $\\frac{a}{b}$");
}

void LatexTextEdit::focusOutEvent(QFocusEvent *event) {
  QTextEdit::focusOutEvent(event);
  emit editingFinished();
}

void LatexTextEdit::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) {
    emit editingCancelled();
    return;
  }
  // Ctrl+Enter or just Enter (without Shift) finishes editing
  if (event->key() == Qt::Key_Return &&
      (event->modifiers() & Qt::ControlModifier)) {
    emit editingFinished();
    return;
  }
  QTextEdit::keyPressEvent(event);
}

// Math-friendly font selection helper with optimized font stack
static QFont selectMathFont(int pointSize) {
  // Priority list of math-friendly fonts with excellent Unicode coverage
  // These fonts are known for high-quality mathematical symbol rendering
  static const QStringList mathFonts = {
      "STIX Two Math",     // Modern STIX font - excellent math support
      "STIX Two Text",     // STIX for text with math
      "STIXGeneral",       // Classic STIX
      "Cambria Math",      // Microsoft's math font
      "Latin Modern Math", // LaTeX default font
      "Asana Math",        // High-quality open-source math font
      "XITS Math",         // Extended STIX
      "DejaVu Serif",      // Good Unicode coverage
      "FreeSerif",         // GNU FreeFont with math symbols
      "Liberation Serif",  // Free serif font
      "Noto Serif",        // Google's universal font
      "Times New Roman",   // Classic fallback
      "serif"              // System serif fallback
  };

  QFontDatabase fontDb;
  for (const QString &fontName : mathFonts) {
    if (fontDb.hasFamily(fontName)) {
      QFont font(fontName, pointSize);
      font.setStyleHint(QFont::Serif, QFont::PreferAntialias);
      font.setHintingPreference(QFont::PreferFullHinting);
      return font;
    }
  }
  // Ultimate fallback with proper styling
  QFont fallback("serif", pointSize);
  fallback.setStyleHint(QFont::Serif, QFont::PreferAntialias);
  return fallback;
}

// LatexTextItem implementation
LatexTextItem::LatexTextItem(QGraphicsItem *parent)
    : QGraphicsObject(parent), textColor_(Qt::white), font_(selectMathFont(14)),
      isEditing_(false), lastScale_(1.0), proxyWidget_(nullptr),
      textEdit_(nullptr)
#ifdef HAVE_QT_WEBENGINE
      ,
      pendingRenderId_(0), katexConnected_(false)
#endif
{
  setFlags(ItemIsSelectable | ItemIsMovable | ItemIsFocusable |
           ItemSendsGeometryChanges);
  setAcceptHoverEvents(true);

  // Initialize with empty content rectangle
  contentRect_ = QRectF(0, 0, MIN_WIDTH, MIN_HEIGHT);
}

LatexTextItem::~LatexTextItem() {
  // The proxyWidget_ is a child of this item, so it will be automatically
  // deleted
  proxyWidget_ = nullptr;
  textEdit_ = nullptr;
}

QRectF LatexTextItem::boundingRect() const {
  if (isEditing_ && textEdit_) {
    return QRectF(0, 0, textEdit_->width() + PADDING * 2,
                  textEdit_->height() + PADDING * 2);
  }
  return contentRect_.adjusted(-PADDING, -PADDING, PADDING, PADDING);
}

void LatexTextItem::paint(QPainter *painter,
                          const QStyleOptionGraphicsItem *option,
                          QWidget * /*widget*/) {
  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setRenderHint(QPainter::TextAntialiasing, true);
  painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

  if (isEditing_) {
    // Draw a subtle background when editing with soft shadow effect
    QRectF bgRect = boundingRect();
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(30, 30, 35, 220));
    painter->drawRoundedRect(bgRect, 6, 6);
    return;
  }

  // Draw the rendered content
  if (!renderedContent_.isNull()) {
    painter->drawPixmap(PADDING, PADDING, renderedContent_);
  } else if (!text_.isEmpty()) {
    // Fallback: draw plain text if rendering failed
    painter->setFont(font_);
    painter->setPen(textColor_);
    painter->drawText(contentRect_, Qt::AlignLeft | Qt::AlignVCenter, text_);
  }

  // Draw selection highlight with refined styling
  if (option->state & QStyle::State_Selected) {
    // Main selection border
    painter->setPen(QPen(QColor(0, 122, 204, 200), 1.5, Qt::SolidLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(boundingRect().adjusted(1, 1, -1, -1), 4, 4);

    // Corner handles for resize hint
    qreal handleSize = 4;
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 122, 204));
    QRectF br = boundingRect();
    painter->drawEllipse(QPointF(br.left(), br.top()), handleSize / 2,
                         handleSize / 2);
    painter->drawEllipse(QPointF(br.right(), br.top()), handleSize / 2,
                         handleSize / 2);
    painter->drawEllipse(QPointF(br.left(), br.bottom()), handleSize / 2,
                         handleSize / 2);
    painter->drawEllipse(QPointF(br.right(), br.bottom()), handleSize / 2,
                         handleSize / 2);
  }
}

void LatexTextItem::setText(const QString &text) {
  if (text_ == text) {
    return;
  }

  // contentRect_ can change during renderContent(), notify scene first.
  prepareGeometryChange();
  text_ = text;
  renderContent();
  update();
  emit textChanged();
}

void LatexTextItem::setTextColor(const QColor &color) {
  textColor_ = color;
  renderContent();
  update();
}

void LatexTextItem::setFont(const QFont &font) {
  if (font_ == font) {
    return;
  }

  // Font changes can alter boundingRect via renderContent().
  prepareGeometryChange();
  font_ = font;
  if (textEdit_) {
    textEdit_->setFont(font_);
  }
  renderContent();
  update();
}

void LatexTextItem::startEditing() {
  if (isEditing_)
    return;

  isEditing_ = true;
  prepareGeometryChange();

  // Create the text edit widget if it doesn't exist
  if (!proxyWidget_) {
    textEdit_ = new LatexTextEdit();
    textEdit_->setFont(font_);
    connect(textEdit_, &LatexTextEdit::editingFinished, this,
            &LatexTextItem::onEditingFinished);
    connect(textEdit_, &LatexTextEdit::editingCancelled, this,
            &LatexTextItem::onEditingCancelled);

    proxyWidget_ = new QGraphicsProxyWidget(this);
    proxyWidget_->setWidget(textEdit_);
  }

  // Set the current text
  textEdit_->setPlainText(text_);

  // Calculate size based on content
  QFontMetrics fm(font_);
  int textWidth = qMax(EDIT_MIN_WIDTH, fm.horizontalAdvance(text_) + 50);
  int textHeight = qMax(EDIT_MIN_HEIGHT, fm.height() * 3);
  textEdit_->setFixedSize(textWidth, textHeight);

  proxyWidget_->setPos(PADDING, PADDING);
  proxyWidget_->setEnabled(true);
  proxyWidget_->show();

  // Set focus after a short delay to ensure widget is ready
  // Use QPointer to safely handle case where this object is destroyed
  QPointer<LatexTextEdit> safeTextEdit = textEdit_;
  QMetaObject::invokeMethod(
      textEdit_,
      [safeTextEdit]() {
        if (safeTextEdit) {
          safeTextEdit->setFocus();
          safeTextEdit->moveCursor(QTextCursor::End);
        }
      },
      Qt::QueuedConnection);

  update();
}

bool LatexTextItem::hasLatex() const {
  static QRegularExpression latexPattern("\\$[^$]+\\$");
  return text_.contains(latexPattern);
}

void LatexTextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    startEditing();
    event->accept();
  } else {
    QGraphicsObject::mouseDoubleClickEvent(event);
  }
}

QVariant LatexTextItem::itemChange(GraphicsItemChange change,
                                   const QVariant &value) {
  if (change == ItemTransformChange || change == ItemTransformHasChanged) {
    // Get the current scale from the transform
    QTransform t = transform();
    qreal currentScale = qSqrt(t.m11() * t.m11() + t.m12() * t.m12());

    // If scale changed significantly, update font size and re-render
    if (qAbs(currentScale - lastScale_) > 0.1 && currentScale > 0.1) {
      // Adjust font size based on scale
      int newFontSize = qRound(14 * currentScale);
      newFontSize = qBound(8, newFontSize, 72); // Clamp to reasonable range

      if (font_.pointSize() != newFontSize) {
        font_.setPointSize(newFontSize);
        lastScale_ = currentScale;

        // Reset transform and re-render at new size
        setTransform(QTransform());
        renderContent();
      }
    }
  }
  return QGraphicsObject::itemChange(change, value);
}

void LatexTextItem::onEditingFinished() { finishEditing(); }

void LatexTextItem::onEditingCancelled() {
  // Revert to previous text and stop editing
  isEditing_ = false;
  if (proxyWidget_) {
    proxyWidget_->hide();
    proxyWidget_->setEnabled(false);
  }
  prepareGeometryChange();
  update();
}

#ifdef HAVE_QT_WEBENGINE
void LatexTextItem::onKatexRenderComplete(quintptr requestId,
                                          const QPixmap &pixmap, bool success) {
  // Check if this is our request
  if (requestId != pendingRenderId_) {
    return;
  }

  pendingRenderId_ = 0;

  if (success && !pixmap.isNull()) {
    prepareGeometryChange();
    renderedContent_ = pixmap;
    contentRect_ = QRectF(0, 0, pixmap.width() / pixmap.devicePixelRatio(),
                          pixmap.height() / pixmap.devicePixelRatio());
    update();
  } else {
    // Fallback to Unicode rendering on failure
    prepareGeometryChange();
    renderedContent_ = renderLatex(text_);
    contentRect_ =
        QRectF(0, 0, renderedContent_.width(), renderedContent_.height());
    update();
  }
}
#endif

void LatexTextItem::finishEditing() {
  if (!isEditing_)
    return;

  isEditing_ = false;

  // Get the text from the editor
  if (textEdit_) {
    text_ = textEdit_->toPlainText();
  }

  // Hide the editor
  if (proxyWidget_) {
    proxyWidget_->hide();
    proxyWidget_->setEnabled(false);
  }

  prepareGeometryChange();
  renderContent();
  update();

  emit editingFinished();
  emit textChanged();
}

void LatexTextItem::renderContent() {
  if (text_.isEmpty()) {
    renderedContent_ = QPixmap();
    contentRect_ = QRectF(0, 0, MIN_WIDTH, MIN_HEIGHT);
    return;
  }

#ifdef HAVE_QT_WEBENGINE
  // Use KaTeX for rendering if available and text contains LaTeX
  if (hasLatex()) {
    // Connect to renderer if not already connected
    if (!katexConnected_) {
      connect(&KatexRenderer::instance(), &KatexRenderer::renderComplete, this,
              &LatexTextItem::onKatexRenderComplete);
      katexConnected_ = true;
    }

    // Extract just the LaTeX content (first match for now)
    static QRegularExpression latexPattern("\\$([^$]+)\\$");
    QRegularExpressionMatch match = latexPattern.match(text_);
    if (match.hasMatch()) {
      QString latex = match.captured(1);

      // Check cache first
      QPixmap cached = KatexRenderer::instance().getCached(
          latex, textColor_, font_.pointSize(), false);
      if (!cached.isNull()) {
        renderedContent_ = cached;
        contentRect_ = QRectF(
            0, 0,
            renderedContent_.width() / renderedContent_.devicePixelRatio(),
            renderedContent_.height() / renderedContent_.devicePixelRatio());
        return;
      }

      // Request async render
      pendingRenderId_ = reinterpret_cast<quintptr>(this);
      KatexRenderer::instance().render(latex, textColor_, font_.pointSize(),
                                       false, pendingRenderId_);

      // Show placeholder while rendering
      contentRect_ = QRectF(0, 0, MIN_WIDTH, MIN_HEIGHT);
      return;
    }
  }
#endif

  // Fallback to Unicode rendering
  renderedContent_ = renderLatex(text_);
  // contentRect_ represents the content area
  contentRect_ =
      QRectF(0, 0, renderedContent_.width(), renderedContent_.height());
}

QPixmap LatexTextItem::renderLatex(const QString &text) {
  // Parse the text and convert LaTeX expressions
  QString htmlContent;
  static QRegularExpression latexPattern("\\$([^$]+)\\$");
  QRegularExpressionMatchIterator it = latexPattern.globalMatch(text);

  int lastEnd = 0;
  while (it.hasNext()) {
    QRegularExpressionMatch match = it.next();
    // Add plain text before the match
    if (match.capturedStart() > lastEnd) {
      QString plainPart = text.mid(lastEnd, match.capturedStart() - lastEnd);
      htmlContent += plainTextToHtmlPreservingNewlines(plainPart);
    }
    // Convert LaTeX to HTML with enhanced styling for math expressions
    QString latex = match.captured(1);
    QString converted = latexToHtml(latex);
    // Wrap LaTeX content in styled span with letter-spacing for better visual
    // distinction
    htmlContent += "<span style='color: " + textColor_.name() +
                   "; letter-spacing: 0.5px;'>" + converted + "</span>";
    lastEnd = match.capturedEnd();
  }

  // Add remaining plain text after the last match
  if (lastEnd < text.length()) {
    htmlContent += plainTextToHtmlPreservingNewlines(text.mid(lastEnd));
  }

  // If no matches were found (no LaTeX), htmlContent will be empty, so use
  // plain text
  if (htmlContent.isEmpty()) {
    htmlContent = plainTextToHtmlPreservingNewlines(text);
  }

  // Render the HTML content using QTextDocument with improved settings
  QTextDocument doc;
  QFont renderFont = font_;
  // Slightly increase font size for better readability of math symbols
  if (hasLatex()) {
    renderFont.setPointSize(renderFont.pointSize() + 1);
  }
  doc.setDefaultFont(renderFont);
  doc.setHtml(htmlContent);
  doc.setTextWidth(-1); // No word wrap

  // Create the pixmap with extra padding for cleaner appearance
  QSizeF size = doc.size();
  int extraPadding = hasLatex() ? 6 : 2; // More padding for math content
  int pixmapWidth =
      qMax(static_cast<int>(std::ceil(size.width())) + extraPadding, MIN_WIDTH);
  int pixmapHeight = qMax(
      static_cast<int>(std::ceil(size.height())) + extraPadding, MIN_HEIGHT);
  QPixmap pixmap(pixmapWidth, pixmapHeight);
  pixmap.fill(Qt::transparent);

  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setRenderHint(QPainter::TextAntialiasing, true);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

  // Set text color with proper context
  QAbstractTextDocumentLayout::PaintContext ctx;
  ctx.palette.setColor(QPalette::Text, textColor_);

  // Center the content slightly for better visual balance
  if (hasLatex()) {
    painter.translate(extraPadding / 2, extraPadding / 2);
  }
  doc.documentLayout()->draw(&painter, ctx);

  return pixmap;
}

QString LatexTextItem::latexToHtml(const QString &latex) {
  QString result = latex;

  // Helper lambda to process regex matches in reverse order (O(n) instead of
  // O(n²))
  auto processMatches =
      [](QString &str, const QRegularExpression &pattern,
         std::function<QString(const QRegularExpressionMatch &)> transform) {
        QList<QPair<qsizetype, QPair<qsizetype, QString>>> replacements;
        QRegularExpressionMatchIterator it = pattern.globalMatch(str);
        while (it.hasNext()) {
          QRegularExpressionMatch match = it.next();
          replacements.append({match.capturedStart(),
                               {match.capturedLength(), transform(match)}});
        }
        // Apply in reverse order
        for (int i = replacements.size() - 1; i >= 0; --i) {
          qsizetype pos = replacements[i].first;
          qsizetype len = replacements[i].second.first;
          const QString &replacement = replacements[i].second.second;
          str.replace(pos, len, replacement);
        }
      };

  // Process \mathbb{X} for blackboard bold
  static QRegularExpression mathbbPattern("\\\\mathbb\\{(\\w)\\}");
  processMatches(result, mathbbPattern, [](const QRegularExpressionMatch &m) {
    QChar ch = m.captured(1)[0];
    return LatexSymbols::mathbb.value(ch, m.captured(1));
  });

  // Process \mathcal{X} for calligraphic
  static QRegularExpression mathcalPattern("\\\\mathcal\\{(\\w)\\}");
  processMatches(result, mathcalPattern, [](const QRegularExpressionMatch &m) {
    QChar ch = m.captured(1)[0];
    return LatexSymbols::mathcal.value(ch, m.captured(1));
  });

  // Process \mathfrak{X} for Fraktur
  static QRegularExpression mathfrakPattern("\\\\mathfrak\\{(\\w)\\}");
  processMatches(result, mathfrakPattern, [](const QRegularExpressionMatch &m) {
    QChar ch = m.captured(1)[0];
    return LatexSymbols::mathfrak.value(ch, m.captured(1));
  });

  // Process fractions: \frac{a}{b} - using proper fraction slash with
  // numerator/denominator
  static QRegularExpression fracPattern("\\\\frac\\{([^}]*)\\}\\{([^}]*)\\}");
  processMatches(result, fracPattern, [](const QRegularExpressionMatch &m) {
    QString num = m.captured(1);
    QString den = m.captured(2);
    // Convert numerator to superscripts
    QString superNum;
    for (QChar ch : num) {
      superNum += LatexSymbols::superscripts.value(ch, QString(ch));
    }
    // Convert denominator to subscripts
    QString subDen;
    for (QChar ch : den) {
      subDen += LatexSymbols::subscripts.value(ch, QString(ch));
    }
    // Use fraction slash (⁄) for proper appearance
    return superNum + QString("⁄") + subDen;
  });

  // Process superscripts: ^{...}
  static QRegularExpression supBracePattern("\\^\\{([^}]*)\\}");
  processMatches(result, supBracePattern, [](const QRegularExpressionMatch &m) {
    QString content = m.captured(1);
    QString superscript;
    for (QChar ch : content) {
      superscript += LatexSymbols::superscripts.value(ch, QString(ch));
    }
    return superscript;
  });

  // Process simple superscripts: ^x
  static QRegularExpression supPattern("\\^(\\w)");
  processMatches(result, supPattern, [](const QRegularExpressionMatch &m) {
    QString ch = m.captured(1);
    if (ch.length() == 1 && LatexSymbols::superscripts.contains(ch[0])) {
      return LatexSymbols::superscripts[ch[0]];
    }
    return ch;
  });

  // Process subscripts: _{...}
  static QRegularExpression subBracePattern("_\\{([^}]*)\\}");
  processMatches(result, subBracePattern, [](const QRegularExpressionMatch &m) {
    QString content = m.captured(1);
    QString subscript;
    for (QChar ch : content) {
      subscript += LatexSymbols::subscripts.value(ch, QString(ch));
    }
    return subscript;
  });

  // Process simple subscripts: _x
  static QRegularExpression subPattern("_(\\w)");
  processMatches(result, subPattern, [](const QRegularExpressionMatch &m) {
    QString ch = m.captured(1);
    if (ch.length() == 1 && LatexSymbols::subscripts.contains(ch[0])) {
      return LatexSymbols::subscripts[ch[0]];
    }
    return ch;
  });

  // Process square root: \sqrt{...} - use proper overline styling hint
  static QRegularExpression sqrtPattern("\\\\sqrt\\{([^}]*)\\}");
  result.replace(sqrtPattern, "√\\1");

  // Process n-th root: \sqrt[n]{...}
  static QRegularExpression nthRootPattern("\\\\sqrt\\[(\\d+)\\]\\{([^}]*)\\}");
  processMatches(result, nthRootPattern, [](const QRegularExpressionMatch &m) {
    QString n = m.captured(1);
    QString content = m.captured(2);
    QString superN;
    for (QChar ch : n) {
      superN += LatexSymbols::superscripts.value(ch, QString(ch));
    }
    return superN + "√" + content;
  });

  // Process simple \sqrt followed by a single character
  static QRegularExpression sqrtSimplePattern("\\\\sqrt(\\w)");
  result.replace(sqrtSimplePattern, "√\\1");

  // Replace LaTeX commands with Unicode symbols (already O(n) using reverse
  // processing)
  static QRegularExpression cmdPattern("\\\\(\\w+)");
  processMatches(result, cmdPattern, [this](const QRegularExpressionMatch &m) {
    return latexCommandToUnicode(m.captured(1));
  });

  // Convert single-letter Latin variables to mathematical italic
  // This gives a more professional mathematical appearance
  // Simple pattern: match isolated Latin letters not preceded by backslash
  static QRegularExpression varPattern("(?<!\\\\)\\b([a-zA-Z])\\b");
  processMatches(result, varPattern, [](const QRegularExpressionMatch &m) {
    QChar ch = m.captured(1)[0];
    return LatexSymbols::mathItalic.value(ch, m.captured(1));
  });

  // Add thin spaces around binary operators for better readability
  // Use Unicode thin space (U+2009) around common operators
  static const QString thinSpace = QString(QChar(0x2009)); // Thin space
  static const QStringList binaryOps = {
      "=", "+", "−", "×", "÷", "±", "∓", "≤", "≥", "≠", "≈", "≡", "∼",
      "⊂", "⊃", "⊆", "⊇", "∈", "∉", "→", "←", "↔", "⇒", "⇐", "⇔"};
  for (const QString &op : binaryOps) {
    result.replace(op, thinSpace + op + thinSpace);
  }

  // Clean up any double thin spaces
  result.replace(thinSpace + thinSpace, thinSpace);

  return result;
}

QString LatexTextItem::latexCommandToUnicode(const QString &cmd) {
  // Check Greek letters
  if (LatexSymbols::greekLetters.contains(cmd)) {
    return LatexSymbols::greekLetters[cmd];
  }

  // Check math symbols
  if (LatexSymbols::mathSymbols.contains(cmd)) {
    return LatexSymbols::mathSymbols[cmd];
  }

  // Return the original command with backslash if not found
  return "\\" + cmd;
}
