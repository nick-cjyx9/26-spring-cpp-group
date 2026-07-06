#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class ShopPage;
}
QT_END_NAMESPACE

class ShopPage : public QWidget
{
    Q_OBJECT

public:
    explicit ShopPage(QWidget *parent = nullptr);
    ~ShopPage();

    void refresh();

private slots:
    void onBuyClicked();
    void onSellClicked();

private:
    Ui::ShopPage *ui;
};
