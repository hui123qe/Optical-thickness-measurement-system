#pragma once

class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QString;

namespace WidgetFactory {

QDoubleSpinBox* createCoordinateInput();
QLabel* createSectionTitle(const QString& title, const QString& subtitle);
QLabel* createStatusBadge(const QString& text, const QString& styleClass);
QPushButton* createPrimaryButton(const QString& text);
QPushButton* createDangerButton(const QString& text);

} // namespace WidgetFactory
