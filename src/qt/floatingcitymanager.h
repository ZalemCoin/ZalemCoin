#ifndef FLOATINGCITYMANAGER_H
#define FLOATINGCITYMANAGER_H

#include "util.h"
#include "sync.h"

#include <QMenu>
#include <QWidget>
#include <QTimer>
#include <QItemSelectionModel>

namespace Ui {
    class FloatingcityManager;

}
class ClientModel;
class WalletModel;
class QAbstractItemView;
class QItemSelectionModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Floatingcity Manager page widget */
class FloatingcityManager : public QWidget
{
    Q_OBJECT

public:
    explicit FloatingcityManager(QWidget *parent = 0);
    ~FloatingcityManager();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);

private:
    QMenu* contextMenu;
    
public slots:
    void updateNodeList();
    void updateAdrenalineNode(QString alias, QString addr, QString privkey, QString txHash, QString txIndex, QString status);
    void on_UpdateButton_clicked();
    void copyAddress();
    void copyPubkey();

signals:

private:
    QTimer *timer;
    Ui::FloatingcityManager *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    CCriticalSection cs_adrenaline;

private slots:
    void showContextMenu(const QPoint&);
    void on_createButton_clicked();
    void on_startButton_clicked();
    void on_startAllButton_clicked();
    void on_stopButton_clicked();
    void on_stopAllButton_clicked();
    void on_tableWidget_2_itemSelectionChanged();
    void on_tabWidget_currentChanged(int index);
    void on_editButton_clicked();
};
#endif // FLOATINGCITYMANAGER_H
