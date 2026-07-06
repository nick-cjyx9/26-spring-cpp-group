#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class CharacterPage;
class InventoryPage;
class ShopPage;
class QuestPage;
class BattlePage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onCharacterClicked();
    void onInventoryClicked();
    void onShopClicked();
    void onQuestClicked();
    void onBattleClicked();
    void onSaveClicked();
    void onLoadClicked();

private:
    Ui::MainWindow *ui;

    CharacterPage *characterPage_ = nullptr;
    InventoryPage *inventoryPage_ = nullptr;
    ShopPage *shopPage_ = nullptr;
    QuestPage *questPage_ = nullptr;
    BattlePage *battlePage_ = nullptr;
};
