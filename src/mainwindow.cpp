#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "vtkActor.h"
#include "vtkNIFTIImageReader.h"
#include "vtkPiecewiseFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkColorTransferFunction.h"
#include "vtkVolumeProperty.h"
#include "vtkVolume.h"
#include "vtkRenderWindowInteractor.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QtDebug>

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mRenderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
    mRenderer(vtkSmartPointer<vtkRenderer>::New()),
    mInteractor(vtkSmartPointer<QVTKInteractor>::New()),
    mNIFTIImageReader(vtkSmartPointer<vtkNIFTIImageReader>::New())
{
    ui->setupUi(this);
    ui->frame_tools->setVisible(false);
    QCoreApplication::setApplicationName(QString("NiftiViewer"));
    setWindowTitle(QCoreApplication::applicationName());

    // Set up rendering
    mRenderWindow->AddRenderer(mRenderer);
    mRenderWindow->SetInteractor(mInteractor);

    //Setting default values of ui widgets
    ui->openGLWidget->SetRenderWindow(mRenderWindow);
    ui->lineEdit_intenisty_range->setValidator(new QIntValidator(0, 100000, this));
    ui->horizontalSlider_maximumOpacity->setSliderPosition(ui->horizontalSlider_maximumOpacity->maximum());
    ui->horizontalSlider_maximumColor->setSliderPosition(ui->horizontalSlider_maximumColor->maximum());
    ui->verticalSlider_Rmax->setSliderPosition(ui->verticalSlider_Rmax->maximum());
    ui->verticalSlider_Gmax->setSliderPosition(ui->verticalSlider_Gmax->maximum());
    ui->verticalSlider_Bmax->setSliderPosition(ui->verticalSlider_Bmax->maximum());
   
    // Set the background color
    mRenderer->SetBackground(0, 0, 0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    // When actionOpen widget triggered open explorator window and wait until user pass a source for file destination.
    // If file is ".nii" type continue to set up model in main render window.

    QString filter = "All Files (*.*) ;; NIFTI File (*.nii)";
    QString fileName = QFileDialog::getOpenFileName(this, "Open a file", "C://", filter);
    QFileInfo fi(fileName);
    QString ext = fi.suffix();
    ui->statusbar->showMessage("Loading file: " + fileName, 5000);
    if (ext != "nii")
    {
        QMessageBox::warning(this, "Warning", "Not supported file");
    }
    if (ext == "nii")
    {
        // Loading raster data from NIFTI file
        mNIFTIImageReader->SetFileName(qPrintable(fileName));
        mNIFTIImageReader->Update();

        //The property describes how the data will look
        vtkSmartPointer<vtkVolumeProperty> volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
        volumeProperty->ShadeOn();
        volumeProperty->SetInterpolationTypeToLinear();

        // The mapper knows how to render the data
        vtkSmartPointer<vtkGPUVolumeRayCastMapper> volumeMapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
        volumeMapper->SetInputConnection(mNIFTIImageReader->GetOutputPort());

        // The volume holds the mapper and the property
        vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
        volume->SetMapper(volumeMapper);
        volume->SetProperty(volumeProperty);

        mRenderer->AddViewProp(volume);
        mRenderer->ResetCamera();
        mRenderWindow->Render();
        mInteractor->Initialize();
        mInteractor->Start();
    }
}

void MainWindow::on_actionMain_properties_window_triggered()
{
    // Set visibility of main tools.

    if (ui->frame_tools->isHidden())
    {
        ui->frame_tools->setVisible(true);
    }
    else { ui->frame_tools->setVisible(false); }
}

void MainWindow::on_actionSave_triggered()
{
    // Save the image captured as a single frame from the main rendering window.

    QString fileName = QFileDialog::getSaveFileName(this, "Save as...", "name", "PNG (*.png);; BMP (*.bmp);;TIFF (*.tiff *.tif);; JPEG (*.jpg *.jpeg)");
    ui->openGLWidget->grabFramebuffer().save(fileName);
}

void MainWindow::on_pushButton_set_intensity_range_pressed()
{
    //3663 for existing project
    int intensity_range = ui->lineEdit_intenisty_range->text().toInt();
    ui->horizontalSlider_minimumColor->setMaximum(intensity_range);
    ui->horizontalSlider_minimumOpacity->setMaximum(intensity_range);
    ui->horizontalSlider_maximumOpacity->setMaximum(intensity_range);
    ui->horizontalSlider_maximumColor->setMaximum(intensity_range);
}

void MainWindow::on_pushButton_apply_pressed()
{
    // Create transfer mapping to opacity
    vtkSmartPointer<vtkPiecewiseFunction> opacityTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();
    opacityTransferFunction->AddPoint(ui->horizontalSlider_minimumOpacity->value(), 0.0);
    opacityTransferFunction->AddPoint(ui->horizontalSlider_maximumOpacity->value(), 1.0);

    // Create transfer mapping to color
    vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction = vtkSmartPointer<vtkColorTransferFunction>::New();
    colorTransferFunction->AddRGBPoint(ui->horizontalSlider_minimumColor->value(), ui->verticalSlider_Rmin->value()/100, ui->verticalSlider_Gmin->value()/100, ui->verticalSlider_Bmin->value()/100);
    colorTransferFunction->AddRGBPoint(ui->horizontalSlider_maximumColor->value(), ui->verticalSlider_Rmax->value()/100, ui->verticalSlider_Gmax->value()/100, ui->verticalSlider_Bmax->value()/100);

    //The property describes how the data will look
    vtkSmartPointer<vtkVolumeProperty> volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->SetColor(colorTransferFunction);
    volumeProperty->SetScalarOpacity(opacityTransferFunction);
    volumeProperty->ShadeOn();
    volumeProperty->SetInterpolationTypeToLinear();

    // The mapper knows how to render the data
    vtkSmartPointer<vtkGPUVolumeRayCastMapper> volumeMapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
    volumeMapper->SetInputConnection(mNIFTIImageReader->GetOutputPort());
    volumeMapper->SetAutoAdjustSampleDistances(true);
    switch (ui->comboBox_blendMode->currentIndex())
    {
    case 0:
        volumeMapper->SetBlendModeToComposite();
        break;
    case 1:
        volumeMapper->SetBlendModeToMaximumIntensity();
        break;
    case 2:
        volumeMapper->SetBlendModeToMinimumIntensity();
        break;
    case 3:
        volumeMapper->SetBlendModeToAverageIntensity();
        break;
    case 4:
        volumeMapper->SetBlendModeToAdditive();
        break;
    case 5:
        volumeMapper->SetBlendModeToIsoSurface();
        break;
    default:
        break;
    }

    // The volume holds the mapper and the property
    vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);

    //Resetting renderer
    mRenderer->RemoveAllViewProps();
    mRenderer->ResetCamera();

    //Rendering edited structure
    mNIFTIImageReader->Update();
    mRenderer->AddViewProp(volume);
    mRenderWindow->Render();
}

void MainWindow::on_actionHelp_triggered()
{
    QMessageBox::about(this, "About program", "A program that enable loading a 3D raster data from Niftii and present volumetric render of the image. Program enable \
tweaking visualization options such as brightness and contrast, and enable image rotation. Designed in QtCreator environment with VTK rendering engine. Made in C++. \n\
Author: Cezary Bujak");
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this, "About Qt");
}

void MainWindow::on_actionExit_triggered()
{
    qApp->exit();
}
