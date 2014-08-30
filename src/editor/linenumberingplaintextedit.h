#ifndef LINENUMBERINGPLAINTEXTEDIT_H
#define LINENUMBERINGPLAINTEXTEDIT_H

#include <QtWidgets/QPlainTextEdit>

class LineNumberArea; // forward declaration

class LineNumberingPlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit LineNumberingPlainTextEdit(QWidget *parent = 0);
    ~LineNumberingPlainTextEdit();

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    QColor lineNumberAreaColor();
    void setLineNumberAreaColor(QColor newColor);

protected:
    QColor _lineNumberAreaColor;
    LineNumberArea *lineNumberArea;
    void resizeEvent(QResizeEvent *event);

signals:

public slots:

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &, int);
};


class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(LineNumberingPlainTextEdit *editor);

    QSize sizeHint() const;

protected:
    void paintEvent(QPaintEvent *event);

private:
    LineNumberingPlainTextEdit *editor;
};


#endif // LINENUMBERINGPLAINTEXTEDIT_H
