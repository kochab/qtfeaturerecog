/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "camera.hh"
#include "imagesettings.hh"

#include <QMediaService>
#include <QCameraViewfinder>
#include <QMediaMetaData>
#include <QCloseEvent>
#include <QAction>
#include <QActionGroup>
#include <QMessageBox>
#include <QMenu>
#include <QPushButton>
#include <QTimer>
#include <QDebug>

Camera::Camera(QWidget *parent) :
    QDialog(parent),
    ui(),
    camera(0),
    imageCapture(0),
    isCapturingImage(false),
    applicationExiting(false)
{
    ui.setupUi(this);
    connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui.takeImageButton, SIGNAL(clicked()), this, SLOT(takeImage()));
    connect(ui.actionSettings, SIGNAL(triggered()), this, SLOT(configureCaptureSettings()));
    connect(ui.actionStartCamera, SIGNAL(triggered()), this, SLOT(startCamera()));
    connect(ui.actionStopCamera, SIGNAL(triggered()), this, SLOT(stopCamera()));

    QActionGroup *videoDevicesGroup = new QActionGroup(this);
    videoDevicesGroup->setExclusive(true);
    foreach(const QByteArray &deviceName, QCamera::availableDevices()) {
        QString description = camera->deviceDescription(deviceName);
        QAction *videoDeviceAction = new QAction(description, videoDevicesGroup);
        videoDeviceAction->setCheckable(true);
        videoDeviceAction->setData(QVariant(deviceName));
        if (cameraDevice.isEmpty()) {
            cameraDevice = deviceName;
            videoDeviceAction->setChecked(true);
        }
        ui.menuDevices->addAction(videoDeviceAction);
    }

    connect(videoDevicesGroup, SIGNAL(triggered(QAction*)), SLOT(updateCameraDevice(QAction*)));
}

Camera::~Camera()
{
    delete imageCapture;
    delete camera;
}

void Camera::init() {
    setCamera(cameraDevice);
    updateCaptureMode();
}

void Camera::setCamera(const QByteArray &cameraDevice)
{
    delete imageCapture;
    delete camera;

    if (cameraDevice.isEmpty())
        camera = new QCamera;
    else
        camera = new QCamera(cameraDevice);

    connect(camera, SIGNAL(stateChanged(QCamera::State)), this, SLOT(updateCameraState(QCamera::State)));
    connect(camera, SIGNAL(error(QCamera::Error)), this, SLOT(displayCameraError()));

    imageCapture = new QCameraImageCapture(camera);
    imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);

    //connect(ui.exposureCompensation, SIGNAL(valueChanged(int)), SLOT(setExposureCompensation(int)));

    camera->setViewfinder(ui.viewFinder);

    updateCameraState(camera->state());
    updateLockStatus(camera->lockStatus(), QCamera::UserRequest);

    connect(imageCapture, SIGNAL(readyForCaptureChanged(bool)), this, SLOT(readyForCapture(bool)));
    connect(imageCapture, SIGNAL(imageCaptured(int,QImage)), this, SLOT(processCapturedImage(int,QImage)));
    connect(imageCapture, SIGNAL(imageSaved(int,QString)), this, SLOT(imageSaved(int,QString)));
    connect(imageCapture, SIGNAL(error(int,QCameraImageCapture::Error,QString)), this,
            SLOT(displayCaptureError(int,QCameraImageCapture::Error,QString)));

    connect(camera, SIGNAL(lockStatusChanged(QCamera::LockStatus, QCamera::LockChangeReason)),
            this, SLOT(updateLockStatus(QCamera::LockStatus, QCamera::LockChangeReason)));

    //ui.captureWidget->setTabEnabled(0, (camera->isCaptureModeSupported(QCamera::CaptureStillImage)));

    updateCaptureMode();
    camera->start();
}

void Camera::keyPressEvent(QKeyEvent * event)
{
    if (event->isAutoRepeat())
        return;

    switch (event->key()) {
    case Qt::Key_CameraFocus:
        displayViewfinder();
        camera->searchAndLock();
        event->accept();
        break;
    case Qt::Key_Camera:
        if (camera->captureMode() == QCamera::CaptureStillImage) {
            takeImage();
        }
        event->accept();
        break;
    default:
        QDialog::keyPressEvent(event);
    }
}

void Camera::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat())
        return;

    switch (event->key()) {
    case Qt::Key_CameraFocus:
        camera->unlock();
        break;
    default:
        QDialog::keyReleaseEvent(event);
    }
}

void Camera::processCapturedImage(int requestId, const QImage& img)
{
    Q_UNUSED(requestId);
    QImage scaledImage = img.scaled(ui.viewFinder->size(),
                                    Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation);

    isCapturingImage = false;

    emit imageCaptured(scaledImage);
    //ui.lastImagePreviewLabel->setPixmap(QPixmap::fromImage(scaledImage));
    

    // Display captured image for 4 seconds.
    //displayCapturedImage();
    //QTimer::singleShot(4000, this, SLOT(displayViewfinder()));
}

void Camera::configureCaptureSettings()
{
    switch (camera->captureMode()) {
    case QCamera::CaptureStillImage:
        configureImageSettings();
        break;
    default:
        break;
    }
}

void Camera::configureImageSettings()
{
    ImageSettings settingsDialog(imageCapture);

    settingsDialog.setImageSettings(imageSettings);

    if (settingsDialog.exec()) {
        imageSettings = settingsDialog.imageSettings();
        imageCapture->setEncodingSettings(imageSettings);
    }
}

void Camera::toggleLock()
{
    switch (camera->lockStatus()) {
    case QCamera::Searching:
    case QCamera::Locked:
        camera->unlock();
        break;
    case QCamera::Unlocked:
        camera->searchAndLock();
    }
}

void Camera::updateLockStatus(QCamera::LockStatus status, QCamera::LockChangeReason reason)
{
    QColor indicationColor = Qt::black;

    switch (status) {
    case QCamera::Searching:
        indicationColor = Qt::yellow;
        //ui.statusbar->showMessage(tr("Focusing..."));
        //ui.lockButton->setText(tr("Focusing..."));
        break;
    case QCamera::Locked:
        indicationColor = Qt::darkGreen;        
        //ui.lockButton->setText(tr("Unlock"));
        //ui.statusbar->showMessage(tr("Focused"), 2000);
        break;
    case QCamera::Unlocked:
        indicationColor = reason == QCamera::LockFailed ? Qt::red : Qt::black;
        //ui.lockButton->setText(tr("Focus"));
        //if (reason == QCamera::LockFailed)
        //    ui.statusbar->showMessage(tr("Focus Failed"), 2000);
    }

    //QPalette palette = ui.lockButton->palette();
    //palette.setColor(QPalette::ButtonText, indicationColor);
    //ui.lockButton->setPalette(palette);
}

void Camera::takeImage()
{
    isCapturingImage = true;
    imageCapture->capture();
}

void Camera::displayCaptureError(int id, const QCameraImageCapture::Error error, const QString &errorString)
{
    Q_UNUSED(id);
    Q_UNUSED(error);
    QMessageBox::warning(this, tr("Image Capture Error"), errorString);
    isCapturingImage = false;
}

void Camera::startCamera()
{
    camera->start();
}

void Camera::stopCamera()
{
    camera->stop();
}

void Camera::updateCaptureMode()
{
    camera->setCaptureMode(QCamera::CaptureStillImage);
}

void Camera::updateCameraState(QCamera::State state)
{
    switch (state) {    
    case QCamera::ActiveState:
        ui.actionStartCamera->setEnabled(false);
        ui.actionStopCamera->setEnabled(true);
        //ui.captureWidget->setEnabled(true);
        ui.actionSettings->setEnabled(true);
        break;
    case QCamera::UnloadedState:
    case QCamera::LoadedState:
        ui.actionStartCamera->setEnabled(true);
        ui.actionStopCamera->setEnabled(false);
        //ui.captureWidget->setEnabled(false);
        ui.actionSettings->setEnabled(false);
    }
}

void Camera::setExposureCompensation(int index)
{
    camera->exposure()->setExposureCompensation(index*0.5);
}

void Camera::displayCameraError()
{
    QMessageBox::warning(this, tr("Camera error"), camera->errorString());
}

void Camera::updateCameraDevice(QAction *action)
{
    setCamera(action->data().toByteArray());
}

void Camera::displayViewfinder()
{
    //ui.stackedWidget->setCurrentIndex(0);
}

void Camera::displayCapturedImage()
{
    //ui.stackedWidget->setCurrentIndex(1);
}

void Camera::readyForCapture(bool ready)
{
    ui.takeImageButton->setEnabled(ready);
}

void Camera::imageSaved(int id, const QString &fileName)
{
    Q_UNUSED(id);
    Q_UNUSED(fileName);
    
    isCapturingImage = false;
    if (applicationExiting)
        close();
}

void Camera::closeEvent(QCloseEvent *event)
{
    if (isCapturingImage) {
        setEnabled(false);
        applicationExiting = true;
        event->ignore();
    } else {
        QTimer::singleShot(1000, camera, SLOT(unload()));
        event->accept();
    }
}
