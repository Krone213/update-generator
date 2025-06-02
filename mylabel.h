// mylabel.h
#ifndef MYLABEL_H
#define MYLABEL_H

#include <QLabel>
#include <QString>

class MyLabel : public QLabel
{
    Q_OBJECT
public:
    explicit MyLabel(QWidget *parent = nullptr);
    explicit MyLabel(const QString &text, QWidget *parent = nullptr);

    // Переопределяем setText, чтобы испустить сигнал
    void setText(const QString &text);
    // Перегрузка для setText(QLatin1StringView), если используется где-то Qt::QLatin1StringView
    void setText(QLatin1StringView text);
    // Переопределяем clear, если он используется
    void clear();


signals:
    void textChanged(const QString &newText); // Сигнал об изменении текста
};

#endif // MYLABEL_H
