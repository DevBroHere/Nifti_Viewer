#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkRenderer.h"
#include "QVTKInteractor.h"
#include "vtkNIFTIImageReader.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::MainWindow* ui;

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> mRenderWindow;
    vtkSmartPointer<vtkRenderer> mRenderer;
    vtkSmartPointer<QVTKInteractor> mInteractor;
    vtkSmartPointer<vtkNIFTIImageReader> mNIFTIImageReader;

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void on_actionOpen_triggered();
    void on_actionMain_properties_window_triggered();
    void on_actionSave_triggered();
    void on_pushButton_apply_pressed();
    void on_pushButton_set_intensity_range_pressed();
    void on_actionHelp_triggered();
    void on_actionAbout_Qt_triggered();
    void on_actionExit_triggered();
};

#endif // MAINWINDOW_H
