#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class CharacterPage;
}
QT_END_NAMESPACE

class CharacterPage : public QWidget
{
    Q_OBJECT

public:
    explicit CharacterPage(QWidget *parent = nullptr);
    ~CharacterPage();

    void refresh();

private:
    Ui::CharacterPage *ui;
};
