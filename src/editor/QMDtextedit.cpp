#include "QMDtextedit.h"
#include "logger.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QTextBlock>
#include <QtGui/QTextLayout>
#include <QtGui/QTextDocumentFragment>
#include <QtWidgets/QApplication>
#include <QtWidgets/QToolTip>
#include <QtCore/QDebug>

QMDTextEdit::QMDTextEdit(QWidget *parent) :
    LineNumberingPlainTextEdit(parent)
{
    this->setUndoRedoEnabled(true);
    this->setMouseTracking(true);

    _indentString = "    ";
    _spacesIndentWidthHint = 4;

    _emphFormatString = "_";
    _strongFormatString = "**";
    _codeFormatString = "`";

    _anchorClickKeyModifiers = Qt::NoModifier;
    _highlightCurrentLine = true;
    _lineHighlightColor = DEF_LINE_HIGHLIGHT_COLOR;

    connect(this, SIGNAL(cursorPositionChanged()),
            this, SLOT(applyHighlightingToCurrentLine()));
    applyHighlightingToCurrentLine();
}

QMDTextEdit::~QMDTextEdit()
{
}

QString QMDTextEdit::indentString()
{
    return _indentString;
}
void QMDTextEdit::setIndentString(QString value)
{
    _indentString = value;
}

int QMDTextEdit::spacesIndentWidthHint()
{
    return _spacesIndentWidthHint;
}
void QMDTextEdit::setSpacesIndentWidthHint(int value)
{
    _spacesIndentWidthHint = value;
}


bool QMDTextEdit::formatEmphasisWithUnderscores()
{
    return (_emphFormatString == "_");
}
void QMDTextEdit::setFormatEmphasisWithUnderscores(bool value)
{
    _emphFormatString = value ? "_" : "*";
}
bool QMDTextEdit::formatStrongWithUnderscores()
{
    return (_strongFormatString == "__");
}
void QMDTextEdit::setFormatStrongWithUnderscores(bool value)
{
    _strongFormatString = value ? "__" : "**";
}


Qt::KeyboardModifiers QMDTextEdit::anchorClickKeyboardModifiers()
{
    return _anchorClickKeyModifiers;
}
void QMDTextEdit::setAnchorClickKeyboardModifiers(Qt::KeyboardModifiers value)
{
    _anchorClickKeyModifiers = value;
}

bool QMDTextEdit::highlightCurrentLine()
{
    return _highlightCurrentLine;
}
void QMDTextEdit::setHighlightCurrentLine(bool value)
{
    _highlightCurrentLine = value;

    if (_highlightCurrentLine)
        applyHighlightingToCurrentLine();
    else
        removeCurrentLineHighlighting();
}

QColor QMDTextEdit::currentLineHighlightColor()
{
    return _lineHighlightColor;
}

void QMDTextEdit::setCurrentLineHighlightColor(QColor value)
{
    _lineHighlightColor = (value.isValid()) ? value : DEF_LINE_HIGHLIGHT_COLOR;
}


bool QMDTextEdit::isBorderChar(QChar character)
{
    return (character.isNull()
            || character.unicode() == QChar::ParagraphSeparator
            || character.unicode() == QChar::LineSeparator
            );
}


bool QMDTextEdit::cursorIsBeforeLineContentStart(QTextCursor cursor)
{
    int curPos = cursor.position();
    QChar character;
    int i = 1;
    do {
        character = this->document()->characterAt(curPos - i);
        if (isBorderChar(character))
            return true;
        i++;
    } while (character == ' ' || character == '\t');
    return false;
}

bool QMDTextEdit::selectionContainsOnlyFullLines(QTextCursor selection)
{
    QChar startChar = this->document()->characterAt(selection.selectionStart()-1);
    QChar lastChar = this->document()->characterAt(selection.selectionEnd()-1);
    QChar afterLastChar = this->document()->characterAt(selection.selectionEnd());
    bool startsAtLineStart = isBorderChar(startChar);
    bool endsAtLineEnd = (afterLastChar.isNull()
                          || lastChar.unicode() == QChar::ParagraphSeparator
                          || lastChar.unicode() == QChar::LineSeparator
                          );
    return (startsAtLineStart && endsAtLineEnd);
}

QList<int> QMDTextEdit::getLineStartPositionsInSelection(QTextCursor selection)
{
    QList<int> lineStarts;
    QTextCursor c(document());
    c.setPosition(selection.selectionStart());
    QTextBlock b = c.block();
    int pos = b.position();
    while (pos < selection.selectionEnd())
    {
        lineStarts.append(pos);
        b = b.next();
        if (!b.isValid())
            break;
        pos = b.position();
    }
    return lineStarts;
}


bool QMDTextEdit::event(QEvent *e)
{
    if (e->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);

        // Indenting with tab/backtab
        if (ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Backtab)
        {
            QTextCursor cursor = this->textCursor();
            if (!cursor.hasSelection())
            {
                if (ke->key() == Qt::Key_Tab)
                    indentAtCursor();
                else // backtab
                    unindentAtCursor();
                return true;
            }
            else if (!cursor.hasComplexSelection())
            {
                // There is a non-complex selection.

                if (!selectionContainsOnlyFullLines(cursor)) {
                    cursor.clearSelection();
                    setTextCursor(cursor);
                    return true;
                }

                if (ke->key() == Qt::Key_Tab)
                    indentSelectedLines();
                else // backtab
                    unindentSelectedLines();
                return true;
            }
        }
    }
    else if (e->type() == QEvent::ToolTip)
    {
        QHelpEvent *he = static_cast<QHelpEvent *>(e);

        QString href = getAnchorHrefAtPos(he->pos());
        if (!href.isNull())
            QToolTip::showText(he->globalPos(), href);
        else {
            QToolTip::hideText();
            e->ignore();
        }
        return true;
    }
    return LineNumberingPlainTextEdit::event(e);
}

QString QMDTextEdit::getAnchorHrefAtPos(QPoint pos)
{
    QTextCursor cur = cursorForPosition(pos);
    // "\n" is not clickable:
    if (document()->characterAt(cur.position()) == QChar::ParagraphSeparator)
        return QString::null;
    QList<QTextLayout::FormatRange> formats = cur.block().layout()->additionalFormats();
    int posInBlock = cur.position() - cur.block().position();
    foreach (QTextLayout::FormatRange range, formats)
    {
        if (posInBlock < range.start || range.start + range.length < posInBlock)
            continue;
        QString href = range.format.anchorHref();
        if (!href.isNull())
            return href;
    }
    return QString::null;
}


void QMDTextEdit::mouseMoveEvent(QMouseEvent *e)
{
    if ((e->modifiers() & _anchorClickKeyModifiers) == _anchorClickKeyModifiers)
    {
        QString href = getAnchorHrefAtPos(e->pos());
        if (href.isNull())
            viewport()->setCursor(Qt::IBeamCursor);
        else
            viewport()->setCursor(Qt::PointingHandCursor);
    }
    else
        viewport()->setCursor(Qt::IBeamCursor);
    LineNumberingPlainTextEdit::mouseMoveEvent(e);
}

void QMDTextEdit::mousePressEvent(QMouseEvent *e)
{
    if ((e->modifiers() & _anchorClickKeyModifiers) == _anchorClickKeyModifiers)
    {
        // The caret is placed upon press (not release) so we disable
        // that here if the mouse is pressed on an anchor:
        QString href = getAnchorHrefAtPos(e->pos());
        if (!href.isNull()) {
            e->ignore();
            return;
        }
    }
    LineNumberingPlainTextEdit::mouseReleaseEvent(e);
}

void QMDTextEdit::mouseReleaseEvent(QMouseEvent *e)
{
    if ((e->modifiers() & _anchorClickKeyModifiers) == _anchorClickKeyModifiers)
    {
        QString href = getAnchorHrefAtPos(e->pos());
        if (!href.isNull()) {
            emit anchorClicked(QUrl(href));
            e->ignore();
            return;
        }
    }
    LineNumberingPlainTextEdit::mouseReleaseEvent(e);
}


int QMDTextEdit::guessNumOfSpacesToDeleteUponUnindenting()
{
    int spacesToDelete = _spacesIndentWidthHint;
    if (spacesToDelete == 0 && _indentString.startsWith(" "))
        spacesToDelete = _indentString.length();
    return spacesToDelete;
}

void QMDTextEdit::indentSelectedLines()
{
    QTextCursor cursor = this->textCursor();

    QList<int> lineStarts = getLineStartPositionsInSelection(cursor);

    // Insert indentString to line start positions
    QTextCursor insertCursor(document());
    insertCursor.beginEditBlock();
    int shift = 0;
    foreach (int lineStart, lineStarts)
    {
        insertCursor.setPosition(lineStart+shift);
        insertCursor.insertText(_indentString);
        shift += _indentString.length();
    }
    insertCursor.endEditBlock();

    if (cursor.hasSelection())
    {
        // Adjust selection to include first added indentString
        int selEnd = cursor.selectionEnd();
        cursor.setPosition(cursor.selectionStart()-_indentString.length());
        cursor.setPosition(selEnd, QTextCursor::KeepAnchor);
        this->setTextCursor(cursor);
    }
}

void QMDTextEdit::unindentSelectedLines()
{
    QTextCursor cursor = this->textCursor();

    QList<int> lineStarts = getLineStartPositionsInSelection(cursor);

    QTextCursor removalCursor(document());
    removalCursor.beginEditBlock();
    int deletedChars = 0;
    foreach (int lineStart, lineStarts)
    {
        int adjustedStart = lineStart - deletedChars;
        removalCursor.setPosition(adjustedStart);
        if (document()->characterAt(adjustedStart) == QChar('\t'))
        {
            // line starts with tab -> just delete the tab.
            removalCursor.deleteChar();
            deletedChars++;
        }
        else if (document()->characterAt(adjustedStart) == QChar(' '))
        {
            // line starts with a space -> must guess how many spaces to delete.
            int spacesToDelete = guessNumOfSpacesToDeleteUponUnindenting();

            while (spacesToDelete > 0 && document()->characterAt(adjustedStart) == QChar(' '))
            {
                removalCursor.deleteChar();
                deletedChars++;
                spacesToDelete--;
            }
        }
    }
    removalCursor.endEditBlock();
}

int QMDTextEdit::numCharsFromCursorToNextTabStop()
{
    QTextCursor cursor = this->textCursor();
    int lineStartPos = cursor.block().position();
    int lineLength = (cursor.position() - lineStartPos);
    int numExistingTabStops = lineLength / _spacesIndentWidthHint;
    int charsAfterTabStop = lineLength - (numExistingTabStops * _spacesIndentWidthHint);
    if (charsAfterTabStop == 0)
        return _spacesIndentWidthHint;
    return _spacesIndentWidthHint - charsAfterTabStop;
}

void QMDTextEdit::indentAtCursor()
{
    QTextCursor cursor = this->textCursor();
    QTextCursor insertCursor(document());
    insertCursor.beginEditBlock();
    insertCursor.setPosition(cursor.position());

    if (_indentString.startsWith(" "))
    {
        int numSpacesToIndent = this->numCharsFromCursorToNextTabStop();
        for (int i = 0; i < numSpacesToIndent; i++)
            insertCursor.insertText(" ");
    }
    else
        insertCursor.insertText(_indentString);

    insertCursor.endEditBlock();
}

void QMDTextEdit::unindentAtCursor()
{
    QTextCursor cursor = this->textCursor();
    QTextCursor removalCursor(document());
    removalCursor.beginEditBlock();

    QTextBlock b = cursor.block();
    int lineStartPos = b.position();
    removalCursor.setPosition(lineStartPos);
    if (document()->characterAt(lineStartPos) == QChar('\t'))
    {
        // line starts with tab -> just delete the tab.
        removalCursor.deleteChar();
    }
    else if (document()->characterAt(lineStartPos) == QChar(' '))
    {
        // line starts with a space -> must guess how many spaces to delete.
        int spacesToDelete = guessNumOfSpacesToDeleteUponUnindenting();

        while (spacesToDelete > 0 && document()->characterAt(lineStartPos) == QChar(' '))
        {
            removalCursor.deleteChar();
            spacesToDelete--;
        }
    }

    removalCursor.endEditBlock();
}


// `.select(QTextCursor::WordUnderCursor)` is not sufficient because it
// considers underscores _ word chars and asterisks * non-word chars
QTextCursor QMDTextEdit::selectWordUnderCursor(QTextCursor cursor)
{
    cursor.select(QTextCursor::WordUnderCursor);

    // Trim out leading & trailing underscores:
    int startAdjust = 0;
    while (document()->characterAt(cursor.selectionStart() + startAdjust) == QChar('_'))
        startAdjust++;
    int endAdjust = 0;
    while (document()->characterAt(cursor.selectionEnd() - 1 - endAdjust) == QChar('_'))
        endAdjust++;

    if (startAdjust != 0 || endAdjust != 0)
    {
        int end = cursor.selectionEnd();
        cursor.setPosition(cursor.selectionStart() + startAdjust);
        cursor.setPosition(end - endAdjust, QTextCursor::KeepAnchor);
    }

    return cursor;
}

QString QMDTextEdit::getSelectedText()
{
    return this->textCursor().selection().toPlainText();
}

QPoint QMDTextEdit::getSelectionStartBaselinePoint()
{
    QTextCursor originalCursor = this->textCursor();

    QTextCursor noSelectionCursor = this->textCursor();
    noSelectionCursor.setPosition(originalCursor.selectionStart());
    this->setTextCursor(noSelectionCursor);

    QPoint point = this->cursorRect().topLeft();

    QFontMetrics metrics(this->font());
    point.setY(point.y() + metrics.ascent());
    point.setX(point.x() + lineNumberAreaWidth());

    this->setTextCursor(originalCursor);
    return point;
}


void QMDTextEdit::toggleFormattingForCurrentSelection(FormatStyle formatStyle)
{
    // Find formatted range

    QTextCursor selectionCursor = textCursor();
    if (!selectionCursor.hasSelection())
        selectionCursor = selectWordUnderCursor(selectionCursor);

    int start = selectionCursor.selectionStart();
    int end = selectionCursor.selectionEnd();
    selectionCursor.clearSelection();

    if (end-start <= 0)
        return;

    // Determine format string to use

    QString formatStr;
    if (formatStyle == Emphasized)
        formatStr = _emphFormatString;
    else if (formatStyle == Strong)
        formatStr = _strongFormatString;
    else if (formatStyle == Code)
        formatStr = _codeFormatString;
    int formatStrLength = formatStr.length();

    QTextCursor tempCursor(textCursor());

    // See if the selection already begins and/or ends with our format string

    tempCursor.setPosition(start - formatStrLength);
    tempCursor.setPosition(start, QTextCursor::KeepAnchor);
    bool startsWithFormatStr = (tempCursor.selectedText() == formatStr);

    tempCursor.setPosition(end);
    tempCursor.setPosition(end + formatStrLength, QTextCursor::KeepAnchor);
    bool endsWithFormatStr = (tempCursor.selectedText() == formatStr);

    // If the selection starts XOR ends with the format string, we're not
    // sure what to do, so let's play it safe and do nothing
    if (startsWithFormatStr && !endsWithFormatStr)
        return;
    if (!startsWithFormatStr && endsWithFormatStr)
        return;

    // Apply formatting

    if (startsWithFormatStr && endsWithFormatStr)
    {
        // remove formatting
        tempCursor.beginEditBlock();

        tempCursor.setPosition(start - formatStrLength);
        tempCursor.setPosition(start, QTextCursor::KeepAnchor);
        tempCursor.removeSelectedText();
        tempCursor.setPosition(end - formatStrLength);
        tempCursor.setPosition(end, QTextCursor::KeepAnchor);
        tempCursor.removeSelectedText();

        tempCursor.endEditBlock();
    }
    else
    {
        // add formatting
        tempCursor.beginEditBlock();

        tempCursor.setPosition(start);
        tempCursor.insertText(formatStr);
        tempCursor.setPosition(end + formatStrLength);
        tempCursor.insertText(formatStr);

        tempCursor.endEditBlock();

        if (textCursor().hasSelection())
        {
            QTextCursor newCursor = textCursor();
            newCursor.setPosition(start + formatStrLength);
            newCursor.setPosition(end + formatStrLength, QTextCursor::KeepAnchor);
            setTextCursor(newCursor);
        }
    }
}


void QMDTextEdit::applyHighlightingToCurrentLine()
{
    if (!_highlightCurrentLine)
        return;

    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly())
    {
        QTextEdit::ExtraSelection selection;

        QTextCursor selCur(textCursor());
        QTextBlock b = selCur.block();
        selCur.setPosition(b.position());
        selCur.setPosition(b.position()+b.length()-1, QTextCursor::KeepAnchor);

        // highlight only if line is not empty
        if (selCur.selectionStart() < selCur.selectionEnd())
        {
            selection.format.setBackground(_lineHighlightColor);
            selection.cursor = selCur;
            extraSelections.append(selection);
        }
    }

    setExtraSelections(extraSelections);
}

void QMDTextEdit::removeCurrentLineHighlighting()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    setExtraSelections(extraSelections);
}
