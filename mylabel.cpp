// mylabel.cpp
#include "mylabel.h"

MyLabel::MyLabel(QWidget *parent) : QLabel(parent)
{
}

MyLabel::MyLabel(const QString &text, QWidget *parent) : QLabel(text, parent)
{
}

void MyLabel::setText(const QString &text)
{
    if (this->text() != text) { // Испускаем сигнал, только если текст действительно изменился
        QLabel::setText(text);
        emit textChanged(text);
    } else {
        // Если текст тот же, но нам все равно нужно "уведомление"
        // (например, если данные те же, но это результат нового выбора ревизии),
        // то можно испустить сигнал безусловно. Но обычно это не требуется.
        // QLabel::setText(text); // все равно вызываем, чтобы QLabel мог обработать, если есть внутренняя логика
        // emit textChanged(text);
    }
}

void MyLabel::setText(QLatin1StringView text)
{
    // QLatin1StringView не имеет прямого сравнения с QString::text(), поэтому конвертируем
    QString newString = QString::fromLatin1(text.data(), text.size());
    if (this->text() != newString) {
        QLabel::setText(text); // Используем оригинальный setText(QLatin1StringView)
        emit textChanged(this->text()); // Передаем QString
    }
}


void MyLabel::clear()
{
    if (!this->text().isEmpty()){ // Испускаем сигнал, только если текст был не пуст
        QLabel::clear();
        emit textChanged(QString()); // Передаем пустую строку как новый текст
    }
}
