/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "mobileappwizardpages.h"

#include "qmlstandaloneapp.h"
#include "qmlstandaloneappwizard.h"
#include "qmlstandaloneappwizardpages.h"
#include "targetsetuppage.h"

#include "qt4projectmanagerconstants.h"

#include <projectexplorer/baseprojectwizarddialog.h>
#include <projectexplorer/customwizard/customwizard.h>
#include <projectexplorer/projectexplorer.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QCoreApplication>
#include <QtGui/QIcon>

namespace Qt4ProjectManager {
namespace Internal {

class QmlStandaloneAppWizardDialog : public AbstractMobileAppWizardDialog
{
    Q_OBJECT

public:
    explicit QmlStandaloneAppWizardDialog(QmlStandaloneAppWizard::WizardType type, QWidget *parent = 0);

private:
    QmlStandaloneAppWizard::WizardType m_type;
    class QmlStandaloneAppWizardSourcesPage *m_qmlSourcesPage;
    friend class QmlStandaloneAppWizard;
};

QmlStandaloneAppWizardDialog::QmlStandaloneAppWizardDialog(QmlStandaloneAppWizard::WizardType type,
                                                           QWidget *parent)
    : AbstractMobileAppWizardDialog(parent)
    , m_type(type)
    , m_qmlSourcesPage(0)
{
    setWindowTitle(m_type == QmlStandaloneAppWizard::NewQmlFile
                       ? tr("New Qt Quick Application")
                       : tr("Qt Quick Application from Existing QML Directory"));
    setIntroDescription(m_type == QmlStandaloneAppWizard::NewQmlFile
                       ? tr("This wizard generates a Qt Quick application project.")
                       : tr("This wizard imports an existing QML directory and creates a Qt Quick application project from it."));

    if (m_type == QmlStandaloneAppWizard::ImportQmlFile) {
        m_qmlSourcesPage = new QmlStandaloneAppWizardSourcesPage;
        m_qmlSourcesPage->setMainQmlFileChooserVisible(true);
        const int qmlSourcesPagePageId = addPage(m_qmlSourcesPage);
        wizardProgress()->item(qmlSourcesPagePageId)->setTitle(tr("QML Sources"));
    }
}

class QmlStandaloneAppWizardPrivate
{
    QmlStandaloneAppWizard::WizardType type;
    class QmlStandaloneApp *standaloneApp;
    class QmlStandaloneAppWizardDialog *wizardDialog;
    friend class QmlStandaloneAppWizard;
};

QmlStandaloneAppWizard::QmlStandaloneAppWizard(WizardType type)
    : AbstractMobileAppWizard(parameters(type))
    , m_d(new QmlStandaloneAppWizardPrivate)
{
    m_d->type = type;
    m_d->standaloneApp = new QmlStandaloneApp;
    m_d->wizardDialog = 0;
}

QmlStandaloneAppWizard::~QmlStandaloneAppWizard()
{
    delete m_d->standaloneApp;
    delete m_d;
}

Core::BaseFileWizardParameters QmlStandaloneAppWizard::parameters(WizardType type)
{
    Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(QLatin1String(Constants::ICON_QML_STANDALONE)));
    parameters.setDisplayName(type == QmlStandaloneAppWizard::NewQmlFile
                              ? tr("Qt Quick Application")
                              : tr("Import Existing QML Directory"));
    parameters.setId(QLatin1String(type == QmlStandaloneAppWizard::NewQmlFile
                                   ? "QA.QMLA Application"
                                   : "QA.QMLB Imported Application"));
    parameters.setDescription(type == QmlStandaloneAppWizard::NewQmlFile
        ? tr("Creates a Qt Quick application that you can deploy to mobile devices.")
        : tr("Imports an existing QML directory and converts it into a "
             "Qt Quick application project. "
             "You can deploy the application to mobile devices."));
    parameters.setCategory(QLatin1String(Constants::QT_APP_WIZARD_CATEGORY));
    parameters.setDisplayCategory(QCoreApplication::translate(Constants::QT_APP_WIZARD_TR_SCOPE,
                                                              Constants::QT_APP_WIZARD_TR_CATEGORY));
    return parameters;
}

AbstractMobileAppWizardDialog *QmlStandaloneAppWizard::createWizardDialogInternal(QWidget *parent) const
{
    m_d->wizardDialog = new QmlStandaloneAppWizardDialog(m_d->type, parent);
    if (m_d->wizardDialog->m_qmlSourcesPage) {
        connect(m_d->wizardDialog->m_qmlSourcesPage,
                SIGNAL(externalModulesChanged(QStringList, QStringList)),
                SLOT(handleModulesChange(QStringList, QStringList)));
    }
    const QList<TargetSetupPage::ImportInfo> &qtVersions
        = TargetSetupPage::importInfosForKnownQtVersions();
    QList<TargetSetupPage::ImportInfo> qmlQtVersions;
    foreach (const TargetSetupPage::ImportInfo &qtVersion, qtVersions) {
        const QString versionString = qtVersion.version->qtVersionString();
        bool isNumber;
        const int majorVersion = versionString.mid(0, 1).toInt(&isNumber);
        if (!isNumber || majorVersion < 4)
            continue;
        const int minorVersion = versionString.mid(2, 1).toInt(&isNumber);
        if (!isNumber || (majorVersion == 4 && minorVersion < 7))
            continue;
        qmlQtVersions << qtVersion;
    }
    m_d->wizardDialog->m_targetsPage->setImportInfos(qmlQtVersions);

    return m_d->wizardDialog;
}

void QmlStandaloneAppWizard::prepareGenerateFiles(const QWizard *w,
    QString *errorMessage) const
{
    Q_UNUSED(errorMessage)
    const QmlStandaloneAppWizardDialog *wizard = qobject_cast<const QmlStandaloneAppWizardDialog*>(w);
    if (wizard->m_qmlSourcesPage) {
        m_d->standaloneApp->setMainQmlFile(wizard->m_qmlSourcesPage->mainQmlFile());
        m_d->standaloneApp->setExternalModules(
                wizard->m_qmlSourcesPage->moduleUris(),
                wizard->m_qmlSourcesPage->moduleImportPaths());
    }
}

bool QmlStandaloneAppWizard::postGenerateFilesInternal(const Core::GeneratedFiles &l,
    QString *errorMessage)
{
    const bool success = ProjectExplorer::CustomProjectWizard::postGenerateOpen(l, errorMessage);
    if (success && m_d->type == QmlStandaloneAppWizard::ImportQmlFile) {
        ProjectExplorer::ProjectExplorerPlugin::instance()->setCurrentFile(0, m_d->standaloneApp->mainQmlFile());
        Core::EditorManager::instance()->openEditor(m_d->standaloneApp->mainQmlFile(),
                                                    QString(), Core::EditorManager::ModeSwitch);
    }
    return success;
}

void QmlStandaloneAppWizard::handleModulesChange(const QStringList &uris, const QStringList &paths)
{
    Q_ASSERT(m_d->wizardDialog->m_qmlSourcesPage);
    QmlStandaloneApp testApp;
    testApp.setExternalModules(uris, paths);
    m_d->wizardDialog->m_qmlSourcesPage->setModulesError(testApp.error());
}

AbstractMobileApp *QmlStandaloneAppWizard::app() const
{
    return m_d->standaloneApp;
}

AbstractMobileAppWizardDialog *QmlStandaloneAppWizard::wizardDialog() const
{
    return m_d->wizardDialog;
}

} // namespace Internal
} // namespace Qt4ProjectManager

#include "qmlstandaloneappwizard.moc"
