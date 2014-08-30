#include "linenumberingplaintextedit.h"
#include "logger.h"

#include <QTextBlock>
#include <QPainter>

LineNumberingPlainTextEdit::LineNumberingPlainTextEdit(QWidget *parent) :
    QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, SIGNAL(blockCountChanged(int)),
            this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)),
            this, SLOT(updateLineNumberArea(QRect,int)));

    _lineNumberAreaColor = QColor(Qt::lightGray).lighter(120);

    updateLineNumberAreaWidth(0);
}

LineNumberingPlainTextEdit::~LineNumberingPlainTextEdit()
{
    delete lineNumberArea;
}


QColor LineNumberingPlainTextEdit::lineNumberAreaColor()
{
    return _lineNumberAreaColor;
}
void LineNumberingPlainTextEdit::setLineNumberAreaColor(QColor newColor)
{
    _lineNumberAreaColor = newColor;
    repaint();
}



void LineNumberingPlainTextEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(),
                                      lineNumberAreaWidth(), cr.height()));
}

#define kLeftMargin 3
#define kRightMargin 3

int LineNumberingPlainTextEdit::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, document()->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    static int nineCharWidth = -1;
    if (nineCharWidth == -1)
        nineCharWidth = fontMetrics().width(QLatin1Char('9'));
    int digitsWidth = nineCharWidth * digits;
    return kLeftMargin + digitsWidth + kRightMargin;
}

void LineNumberingPlainTextEdit::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void LineNumberingPlainTextEdit::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}


#define kMinTextLightness 50
#define kMaxTextLightness 205

void LineNumberingPlainTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QColor backgroundColor = _lineNumberAreaColor;
    bool lightBackground = (128 <= backgroundColor.lightness());
    QColor lineNumberTextColor = lightBackground
                                 ? backgroundColor.darker(180)
                                 : backgroundColor.lighter(180);
    QColor borderLineColor = lightBackground
                             ? backgroundColor.darker(130)
                             : backgroundColor.lighter(130);

    if (lineNumberTextColor.lightness() < kMinTextLightness)
        lineNumberTextColor.setHsl(lineNumberTextColor.hslHue(), lineNumberTextColor.hslSaturation(), kMinTextLightness);
    else if (kMaxTextLightness < lineNumberTextColor.lightness())
        lineNumberTextColor.setHsl(lineNumberTextColor.hslHue(), lineNumberTextColor.hslSaturation(), kMaxTextLightness);

    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), backgroundColor);

    painter.setPen(borderLineColor);
    painter.drawLine(event->rect().topRight(), event->rect().bottomRight());

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString lineNumberString = QString::number(blockNumber + 1);
            painter.setPen(lineNumberTextColor);
            painter.setFont(this->font());
            painter.drawText(0, top,
                             lineNumberArea->width() - kRightMargin,
                             fontMetrics().height(),
                             Qt::AlignRight|Qt::AlignBottom, lineNumberString);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}




LineNumberArea::LineNumberArea(LineNumberingPlainTextEdit *ed) : QWidget(ed)
{
    editor = ed;
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    editor->lineNumberAreaPaintEvent(event);
}

QSize LineNumberArea::sizeHint() const {
    return QSize(editor->lineNumberAreaWidth(), 0);
}
