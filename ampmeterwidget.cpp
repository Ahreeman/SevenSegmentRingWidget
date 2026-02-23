#include "AmpMeterWidget.h"

#include <QDateTime>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QtMath>

namespace
{
constexpr double kMinValue = 0.0;
constexpr double kMaxValue = 99.9;

bool segmentOn(int digit, int segment)
{
    // Segment order: A, B, C, D, E, F, G
    static const bool table[10][7] = {
        {true, true, true, true, true, true, false},   // 0
        {false, true, true, false, false, false, false}, // 1
        {true, true, false, true, true, false, true},  // 2
        {true, true, true, true, false, false, true},  // 3
        {false, true, true, false, false, true, true}, // 4
        {true, false, true, true, false, true, true},  // 5
        {true, false, true, true, true, true, true},   // 6
        {true, true, true, false, false, false, false}, // 7
        {true, true, true, true, true, true, true},    // 8
        {true, true, true, true, false, true, true}    // 9
    };

    if (digit < 0 || digit > 9 || segment < 0 || segment > 6) {
        return false;
    }
    return table[digit][segment];
}
} // namespace

AmpMeterWidget::NumberFormatMode AmpMeterWidget::numberFormatMode() const
{
    return m_numberFormatMode;
}

AmpMeterWidget::AmpMeterWidget(QWidget *parent)
    : QWidget(parent)
    , m_value(0.0)
    , m_demoAnimationEnabled(false)
    , m_animTimer(new QTimer(this))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(m_animTimer, &QTimer::timeout, this, [this]() {
        const double next = 50.0 + 45.0 * qSin(static_cast<double>(QDateTime::currentMSecsSinceEpoch()) / 1200.0);
        setValue(next);
    });
}

double AmpMeterWidget::value() const
{
    return m_value;
}

bool AmpMeterWidget::demoAnimationEnabled() const
{
    return m_demoAnimationEnabled;
}

void AmpMeterWidget::setNumberFormatMode(NumberFormatMode mode)
{
    if(m_numberFormatMode == mode)
        return;
    m_numberFormatMode = mode;
    update();
}

void AmpMeterWidget::setValue(double newValue)
{
    const double bounded = qBound(kMinValue, newValue, kMaxValue);
    if (qFuzzyCompare(m_value, bounded)) {
        return;
    }
    m_value = bounded;
    update();
}

void AmpMeterWidget::setDemoAnimationEnabled(bool enabled)
{
    if (m_demoAnimationEnabled == enabled) {
        return;
    }

    m_demoAnimationEnabled = enabled;
    if (enabled) {
        m_animTimer->start(33);
        return;
    }

    m_animTimer->stop();
}

QSize AmpMeterWidget::minimumSizeHint() const
{
    return {260, 260};
}

void AmpMeterWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(rect(), QColor(255, 255, 255));

    const QRectF drawingBounds = rect().adjusted(12.0, 12.0, -12.0, -12.0);
    drawRing(painter, drawingBounds);
    drawDisplay(painter, drawingBounds);
}

void AmpMeterWidget::drawRing(QPainter &painter, const QRectF &bounds)
{
    const QPointF center = bounds.center();
    const double radius = qMin(bounds.width(), bounds.height()) * 0.54;

    QPen ringTrack(QColor(201, 209, 217), radius * 0.12, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(ringTrack);
    painter.setBrush(Qt::NoBrush);
    painter.drawArc(QRectF(center.x() - radius, center.y() - radius, radius * 2.0, radius * 2.0), 30 * 16, 300 * 16);

    const double ratio = m_value / kMaxValue;
    const int spanAngle = static_cast<int>(300.0 * ratio * 16.0);

    QConicalGradient glow(center, -90.0);
    glow.setColorAt(0.0, QColor(46, 91, 255));
    glow.setColorAt(0.55, QColor(74, 144, 255));
    glow.setColorAt(1.0, QColor(107, 182, 255));

    QPen valuePen(QBrush(glow), radius * 0.12, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(valuePen);
    painter.drawArc(QRectF(center.x() - radius, center.y() - radius, radius * 2.0, radius * 2.0), 30 * 16, spanAngle);

    QPen tickPen(QColor(128, 134, 150, 130), 1.0);
    painter.setPen(tickPen);
    for (int i = 0; i <= 10; ++i) {
        const double t = static_cast<double>(i) / 10.0;
        const double deg = -60.0 + 300.0 * t;
        const double rad = qDegreesToRadians(deg);
        const QPointF p1(center.x() + (radius * 0.76) * qCos(rad), center.y() + (radius * 0.76) * qSin(rad));
        const QPointF p2(center.x() + (radius * 0.87) * qCos(rad), center.y() + (radius * 0.87) * qSin(rad));
        painter.drawLine(p1, p2);
    }
}

void AmpMeterWidget::drawDisplay(QPainter &painter, const QRectF &bounds)
{
    const double boxWidth = bounds.width() * 0.56;
    const double boxHeight = bounds.height() * 0.26;
    const QRectF displayRect(bounds.center().x() - boxWidth / 2.0,
                             bounds.center().y() - boxHeight / 2.0,
                             boxWidth,
                             boxHeight);

    QPainterPath displayPath;
    displayPath.addRoundedRect(displayRect, boxHeight * 0.2, boxHeight * 0.2);

    QLinearGradient panelGrad(displayRect.topLeft(), displayRect.bottomLeft());
    panelGrad.setColorAt(0.0, QColor(230, 232, 235));
    panelGrad.setColorAt(1.0, QColor(210, 214, 219));

    painter.setPen(QPen(QColor(167, 175, 184), 1.2));
    painter.setBrush(panelGrad);
    painter.drawPath(displayPath);

    const QString text = displayText();
    const int digitCount = text.size();

    const double padding = boxHeight * 0.18;
    const double availableWidth = displayRect.width() - 2.0 * padding;
    const double digitWidth = availableWidth / digitCount;
    const QRectF digitArea(displayRect.left() + padding,
                           displayRect.top() + padding,
                           availableWidth,
                           displayRect.height() - 2.0 * padding);

    for (int i = 0; i < digitCount; ++i) {
        if (text[i] == '.') {
            continue;
        }

        const QRectF digitRect(digitArea.left() + i * digitWidth,
                               digitArea.top(),
                               digitWidth,
                               digitArea.height());

        const bool withDecimalPoint = (i + 1 < text.size() && text[i + 1] == '.');
        drawDigit(painter, digitRect, text[i].digitValue(), withDecimalPoint);
    }

    painter.setPen(QColor(126, 164, 189));
    QFont unitFont = painter.font();
    unitFont.setPointSizeF(boxHeight * 0.12);
    unitFont.setBold(true);
    painter.setFont(unitFont);
    painter.drawText(displayRect.adjusted(0.0, 0.0, -padding * 0.7, -padding * 0.4), Qt::AlignRight | Qt::AlignBottom, "A");
}

void AmpMeterWidget::drawDigit(QPainter &painter, const QRectF &digitRect, int digit, bool withDecimalPoint)
{
    const qreal w = digitRect.width();
    const qreal h = digitRect.height();
    const qreal t = qMin(w, h) * 0.14;

    const QColor onColor(47, 52, 58);
    const QColor offColor(47, 52, 58, 40);

    const QRectF a(digitRect.left() + t, digitRect.top(), w - 2.0 * t, t);
    const QRectF b(digitRect.right() - t, digitRect.top() + t, t, h / 2.0 - t * 1.2);
    const QRectF c(digitRect.right() - t, digitRect.center().y() + t * 0.2, t, h / 2.0 - t * 1.2);
    const QRectF d(digitRect.left() + t, digitRect.bottom() - t, w - 2.0 * t, t);
    const QRectF e(digitRect.left(), digitRect.center().y() + t * 0.2, t, h / 2.0 - t * 1.2);
    const QRectF f(digitRect.left(), digitRect.top() + t, t, h / 2.0 - t * 1.2);
    const QRectF g(digitRect.left() + t, digitRect.center().y() - t / 2.0, w - 2.0 * t, t);

    const QRectF segments[7] = {a, b, c, d, e, f, g};
    const bool horizontal[7] = {true, false, false, true, false, false, true};

    for (int seg = 0; seg < 7; ++seg) {
        const bool lit = segmentOn(digit, seg);
        painter.setPen(Qt::NoPen);
        painter.setBrush(lit ? onColor : offColor);
        drawSegment(painter, segments[seg], horizontal[seg], lit);
    }

    if (withDecimalPoint) {
        painter.setBrush(onColor);
        painter.drawEllipse(QRectF(digitRect.right() + t * 0.2, digitRect.bottom() - t * 1.5, t * 0.9, t * 0.9));
    }
}

void AmpMeterWidget::drawSegment(QPainter &painter, const QRectF &rect, bool horizontal, bool lit)
{
    Q_UNUSED(horizontal)

    QPainterPath path;
    const qreal radius = rect.height() * 0.4;
    path.addRoundedRect(rect, radius, radius);

    painter.drawPath(path);

    if (lit) {
        QPen glowPen(QColor(169, 255, 227, 110), 1.0);
        painter.setPen(glowPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }
}

QString AmpMeterWidget::displayText() const
{
    if(m_numberFormatMode == NumberFormatMode::Decimal1)
    {
        const double v = qBound(0.0, m_value, 9.9);
        return QString::number(v, 'f', 1);
    }else if(m_numberFormatMode == NumberFormatMode::Decimal2)
    {
        return QString::number(m_value, 'f', 1).rightJustified(4, '0');
    }else{
        const int iv = qRound(m_value);
        return QString::number(iv).rightJustified(3, '0');
    }

}
