#include "appimageupdater.h"

#include <QTimer>

#include "appimage/update.h"

AppImageUpdater::AppImageUpdater(const QString &id, const QString &path, QObject *parent)
    : QObject(parent), id_(id), path_(path)
{
    isFinishedWithNoError_ = false;
    errorString_ = "";
    updater_ = new appimage::update::Updater(path_.toStdString(), false);
}

AppImageUpdater::~AppImageUpdater()
{
    delete updater_;
}

QString AppImageUpdater::id() const
{
    return id_;
}

QString AppImageUpdater::path() const
{
    return path_;
}

bool AppImageUpdater::isFinishedWithNoError() const
{
    return isFinishedWithNoError_;
}

QString AppImageUpdater::errorString() const
{
    return errorString_;
}

QString AppImageUpdater::describeAppImage() const
{
    std::string description;
    if (updater_->describeAppImage(description)) {
        return QString::fromStdString(description);
    }
    return QString();
}

bool AppImageUpdater::checkForChanges() const
{
    bool updateAvailable;
    if (updater_->checkForChanges(updateAvailable)) {
        return updateAvailable;
    }
    return false;
}

void AppImageUpdater::start()
{
    isFinishedWithNoError_ = false;
    errorString_ = "";

    if (!updater_->start()) {
        emit finished(this);
        return;
    }

    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &AppImageUpdater::checkProgress);
    connect(this, &AppImageUpdater::finished, timer, &QTimer::stop);
    timer->start(100);
}

void AppImageUpdater::checkProgress()
{
    if (!updater_->isDone()) {
        double progress;
        if (updater_->progress(progress)) {
            emit updateProgress(id_, progress);
        }
        return;
    }

    if (updater_->hasError()) {
        std::string message;
        while (updater_->nextStatusMessage(message)) {
            errorString_ += QString::fromStdString(message) + "\n";
        }
        emit finished(this);
        return;
    }

    isFinishedWithNoError_ = true;
    emit finished(this);
}
