#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class QuestPage;
}
QT_END_NAMESPACE

class QuestPage : public QWidget
{
    Q_OBJECT

public:
    explicit QuestPage(QWidget *parent = nullptr);
    ~QuestPage();

    void refresh();

private slots:
    void onAcceptClicked();
    void onCompleteClicked();
    void onRewardClicked();

private:
    Ui::QuestPage *ui;
    void populateList();
};
