/**
 * @file pdf_search_bar.h
 * @brief Floating search bar overlay for PDF text search.
 */
#ifndef PDF_SEARCH_BAR_H
#define PDF_SEARCH_BAR_H

#ifdef HAVE_QT_PDF

#include <QFrame>

class QLabel;
class QLineEdit;
class QPushButton;

/**
 * @brief Floating search bar widget for PDF text search.
 *
 * Provides a compact search interface with text input, prev/next
 * navigation, match count display, and close button. Designed to
 * float over the top-right corner of its parent widget.
 */
class PdfSearchBar : public QFrame {
  Q_OBJECT

public:
  explicit PdfSearchBar(QWidget *parent = nullptr);
  ~PdfSearchBar() override = default;

  /**
   * @brief Show the search bar and focus the input field
   */
  void activate();

  /**
   * @brief Hide the search bar and clear search state
   */
  void deactivate();

  /**
   * @brief Update the match count display
   * @param current Current match index (1-based), 0 if no matches
   * @param total Total number of matches
   */
  void setMatchInfo(int current, int total);

  /**
   * @brief Get the current search text
   */
  QString searchText() const;

signals:
  /** @brief Emitted when the search text changes */
  void searchTextChanged(const QString &text);

  /** @brief Emitted when user clicks "Next" or presses Enter */
  void findNext();

  /** @brief Emitted when user clicks "Previous" or presses Shift+Enter */
  void findPrevious();

  /** @brief Emitted when the search bar is closed */
  void closed();

protected:
  bool eventFilter(QObject *obj, QEvent *event) override;

private:
  QLineEdit *searchInput_;
  QPushButton *prevButton_;
  QPushButton *nextButton_;
  QLabel *matchLabel_;
  QPushButton *closeButton_;

  void positionInParent();
};

#endif // HAVE_QT_PDF

#endif // PDF_SEARCH_BAR_H
