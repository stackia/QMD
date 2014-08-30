#ifndef QMDTEXTEDIT_H
#define QMDTEXTEDIT_H

#include <QtCore/QEvent>
#include <QtCore/QUrl>
#include <QtWidgets/QPlainTextEdit>

#include "linenumberingplaintextedit.h"
#include "defines.h"

class QMDTextEdit : public LineNumberingPlainTextEdit
{
    Q_OBJECT
public:
    explicit QMDTextEdit(QWidget *parent = 0);
    ~QMDTextEdit();

    QString indentString();
    void setIndentString(QString value);

    /** @brief How many spaces to consider an indentation.
      *
      * A "hint" for determining how many spaces to delete upon
      * unindenting a line that starts with spaces. The value 0 denotes
      * "no hint".
      */
    int spacesIndentWidthHint();
    void setSpacesIndentWidthHint(int value);

    /** @brief Keyboard modifiers required for clicking on anchors. */
    Qt::KeyboardModifiers anchorClickKeyboardModifiers();
    void setAnchorClickKeyboardModifiers(Qt::KeyboardModifiers value);

    bool highlightCurrentLine();
    void setHighlightCurrentLine(bool value);

    QColor currentLineHighlightColor();
    void setCurrentLineHighlightColor(QColor value);

    QTextCursor selectWordUnderCursor(QTextCursor cursor);
    QString getSelectedText();
    QPoint getSelectionStartBaselinePoint();

    enum FormatStyle
    {
        Emphasized,
        Strong,
        Code
    };
    void toggleFormattingForCurrentSelection(FormatStyle formatStyle);

    bool formatEmphasisWithUnderscores();
    void setFormatEmphasisWithUnderscores(bool value);
    bool formatStrongWithUnderscores();
    void setFormatStrongWithUnderscores(bool value);

protected:
    QString _emphFormatString;
    QString _strongFormatString;
    QString _codeFormatString;
    QString _indentString;
    int _spacesIndentWidthHint;
    Qt::KeyboardModifiers _anchorClickKeyModifiers;
    bool _highlightCurrentLine;
    QColor _lineHighlightColor;

    bool event(QEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

    QString getAnchorHrefAtPos(QPoint pos);
    bool isBorderChar(QChar character);
    bool cursorIsBeforeLineContentStart(QTextCursor cursor);
    bool selectionContainsOnlyFullLines(QTextCursor selection);
    QList<int> getLineStartPositionsInSelection(QTextCursor selection);
    int guessNumOfSpacesToDeleteUponUnindenting();
    void indentSelectedLines();
    void unindentSelectedLines();
    int numCharsFromCursorToNextTabStop();
    void indentAtCursor();
    void unindentAtCursor();

signals:
    void anchorClicked(QUrl url);

private slots:
    void applyHighlightingToCurrentLine();
    void removeCurrentLineHighlighting();
};


#endif // QMDTEXTEDIT_H
