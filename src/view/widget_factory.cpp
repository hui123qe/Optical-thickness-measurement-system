#include "widget_factory.h"

#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QString>

namespace WidgetFactory {

QDoubleSpinBox* createCoordinateInput()
{
    QDoubleSpinBox* input = new QDoubleSpinBox;
    input->setDecimals(3);
    input->setRange(-1000000.0, 1000000.0);
    input->setSuffix(QStringLiteral(" mm"));
    input->setSingleStep(0.1);
    return input;
}

QLabel* createSectionTitle(const QString& title, const QString& subtitle)
{
    QLabel* label = new QLabel(QStringLiteral("<span class='title'>%1</span><br><span class='subtitle'>%2</span>")
                                   .arg(title, subtitle));
    label->setTextFormat(Qt::RichText);
    label->setProperty("role", "sectionTitle");
    return label;
}

QLabel* createStatusBadge(const QString& text, const QString& styleClass)
{
    QLabel* badge = new QLabel(text);
    badge->setProperty("role", "badge");
    badge->setProperty("state", styleClass);
    badge->setAlignment(Qt::AlignCenter);
    return badge;
}

QPushButton* createPrimaryButton(const QString& text)
{
    QPushButton* button = new QPushButton(text);
    button->setProperty("role", "primary");
    return button;
}

QPushButton* createDangerButton(const QString& text)
{
    QPushButton* button = new QPushButton(text);
    button->setProperty("role", "danger");
    return button;
}

} // namespace WidgetFactory
