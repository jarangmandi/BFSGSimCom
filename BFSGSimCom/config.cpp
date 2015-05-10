#include "config.h"
#include <QtCore/QSettings>

/*
 * ini file location: %APPDATA%\TS3Client
 */

#define CONF_FILE				"test_plugin" // ini file
#define CONF_APP				"TS3Client"   // application folder
#define CONF_OBJ(x)			QSettings x(QSettings::IniFormat, QSettings::UserScope, CONF_APP, CONF_FILE);

Config::Config()
{
    bool bl;
	setupUi(this);
	CONF_OBJ(cfg);

    bl = cfg.value("mode/disabled").toBool();
    rbDisabled->setChecked(bl);

    bl = cfg.value("mode/manual").toBool();
    rbEasyMode->setChecked(bl);

    bl = cfg.value("mode/auto").toBool();
    rbExpertMode->setChecked(bl);

    if (!(rbDisabled->isChecked() || rbEasyMode->isChecked() || rbExpertMode->isChecked()))
    {
        rbDisabled->setChecked(true);
    }


	//lineEdit->setText(cfg.value("global/test").toString());

 //   treeWidget->setColumnCount(4);

 //   QList<QTreeWidgetItem *> items;
 //   QStringList strList;

 //   for (int i = 0; i < 10; ++i)
 //   {
 //       strList.clear();
 //       strList.append(QString("item: %1").arg(i));
 //       strList.append(QString("two"));
 //       strList.append(QString("three"));
 //       strList.append(QString("four"));

 //       items.append(new QTreeWidgetItem((QTreeWidget*)0, strList));

 //   }
 //   treeWidget->insertTopLevelItems(0, items);

}

Config::~Config()
{

}

void Config::accept()
{
	CONF_OBJ(cfg);

    bool bl;

    bl = rbDisabled->isChecked();
    cfg.setValue("mode/disabled", bl);

    bl = rbEasyMode->isChecked();
    cfg.setValue("mode/manual", bl);

    bl = rbExpertMode->isChecked();
    cfg.setValue("mode/auto", bl);


	QDialog::accept();
}

void Config::reject()
{
	QDialog::reject();
}