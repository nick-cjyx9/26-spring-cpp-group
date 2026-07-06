#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class InventoryPage;
}
QT_END_NAMESPACE

class InventoryPage : public QWidget
{
    Q_OBJECT

public:
    explicit InventoryPage(QWidget *parent = nullptr);
    ~InventoryPage();

    void refresh();

private slots:
    void onUseClicked();
    void onDropClicked();

private:
    Ui::InventoryPage *ui;
};
