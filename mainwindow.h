#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileInfo>
#include <QListWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;

private slots:
    void on_startButton_clicked();
    void on_clearButton_clicked();

    void on_actionABOUT_triggered();
    void on_actionHelp_triggered();

    void on_infilesList_itemDoubleClicked(QListWidgetItem *item);

private:
    Ui::MainWindow *ui;

    QFileInfo outdir;
    QList<QString> infiles;
};
#endif // MAINWINDOW_H
