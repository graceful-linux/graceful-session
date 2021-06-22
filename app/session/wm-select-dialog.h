#ifndef WMSELECTDIALOG_H
#define WMSELECTDIALOG_H
#include <QDialog>
#include "window-manager.h"

class QModelIndex;

namespace Ui {
class WMSelectDialog;
}

class WMSelectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit WMSelectDialog(const WindowManagerList &availableWindowManagers,
                            QWidget *parent = nullptr);
    ~WMSelectDialog() override;
    QString windowManager() const;

public Q_SLOTS:
    void done(int) override;

private Q_SLOTS:
    void selectFileDialog(const QModelIndex &index);
    void changeBtnStatus(const QModelIndex &index);

private:
    Ui::WMSelectDialog *ui;

    void addWindowManager(const WindowManager &wm);
};
#endif // WMSELECTDIALOG_H
