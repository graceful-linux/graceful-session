#include "wm-select-dialog.h"

#include "ui_wm-select-dialog.h"

#include <graceful/globals.h>

#include <QTreeWidget>
#include <QVariant>
#include <stdlib.h>
#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QDebug>

#define TYPE_ROLE   Qt::UserRole + 1
#define SELECT_DLG_TYPE 12345

WMSelectDialog::WMSelectDialog(const WindowManagerList &availableWindowManagers, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WMSelectDialog)
{
    qApp->setStyle("plastique");
    ui->setupUi(this);
    setModal(true);
    connect(ui->wmList, &QTreeWidget::doubleClicked, this, &WMSelectDialog::accept);
    connect(ui->wmList, &QTreeWidget::clicked,       this, &WMSelectDialog::selectFileDialog);
    connect(ui->wmList, &QTreeWidget::activated,     this, &WMSelectDialog::changeBtnStatus);

    for (const WindowManager &wm : availableWindowManagers) {
        addWindowManager(wm);
    }

    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText(0, tr("Other ..."));
    item->setText(1, tr("Choose your favorite one."));
    item->setData(1, TYPE_ROLE, SELECT_DLG_TYPE);

    ui->wmList->setCurrentItem(ui->wmList->topLevelItem(0));
    ui->wmList->addTopLevelItem(item);
}


WMSelectDialog::~WMSelectDialog()
{
    delete ui;
}


void WMSelectDialog::done( int r )
{
    QString wm = windowManager();
    if (r==1 && !wm.isEmpty() && findProgram(wm)) {
        QDialog::done( r );
        close();
    }
}


QString WMSelectDialog::windowManager() const
{
    QTreeWidgetItem *item = ui->wmList->currentItem();
    if (item)
        return item->data(0, Qt::UserRole).toString();

    return QString();
}


void WMSelectDialog::addWindowManager(const WindowManager &wm)
{
    QTreeWidgetItem *item = new QTreeWidgetItem();

    item->setText(0, wm.name);
    item->setText(1, wm.comment);
    item->setData(0, Qt::UserRole, wm.command);

    ui->wmList->addTopLevelItem(item);
}


void WMSelectDialog::selectFileDialog(const QModelIndex &/*index*/)
{
    QTreeWidget *wmList = ui->wmList;
    QTreeWidgetItem *item = wmList->currentItem();
    if (item->data(1, TYPE_ROLE) != SELECT_DLG_TYPE)
        return;

    QString fname = QFileDialog::getOpenFileName(this, QString(), "/usr/bin/");
    if (fname.isEmpty())
        return;

    QFileInfo fi(fname);
    if (!fi.exists() || !fi.isExecutable())
        return;

    QTreeWidgetItem *wmItem = new QTreeWidgetItem();

    wmItem->setText(0, fi.baseName());
    wmItem->setData(0, Qt::UserRole, fi.absoluteFilePath());
    wmList->insertTopLevelItem(wmList->topLevelItemCount() -1, wmItem);
    ui->wmList->setCurrentItem(wmItem);
}

void WMSelectDialog::changeBtnStatus(const QModelIndex &/*index*/)
{
    QString wm = windowManager();
    ui->buttonBox->setEnabled(!wm.isEmpty() && findProgram(wm));
}
