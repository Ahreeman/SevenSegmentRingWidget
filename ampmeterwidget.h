#ifndef AMPMETERWIDGET_H
#define AMPMETERWIDGET_H

#include <QWidget>
#include <QDateTime>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QtMath>



class AmpMeterWidget : public QWidget
{
    Q_OBJECT
    enum class NumberFormatMode {
        Integer,
        Decimal1,
        Decimal2
    };
    NumberFormatMode numberFormatMode() const;

public:
    explicit AmpMeterWidget(QWidget *parent = nullptr);

    double value() const;
    bool demoAnimationEnabled() const;
    void setNumberFormatMode(enum NumberFormatMode mode);


public slots:
    void setValue(double newValue);
    void setDemoAnimationEnabled(bool enabled);

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize minimumSizeHint() const override;

private:
    void drawRing(QPainter &painter, const QRectF &bounds);
    void drawDisplay(QPainter &painter, const QRectF &bounds);
    void drawDigit(QPainter &painter, const QRectF &digitRect, int digit, bool withDecimalPoint = false);
    void drawSegment(QPainter &painter, const QRectF &rect, bool horizontal, bool lit);
    QString displayText() const;

private:
    double m_value;
    bool m_demoAnimationEnabled;
    QTimer *m_animTimer;
    NumberFormatMode m_numberFormatMode = NumberFormatMode::Decimal1;

};

#endif // AMPMETERWIDGET_H


