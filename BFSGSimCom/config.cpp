#include <QtCore/QSettings>

#include <sstream>

#include "config.h"

#include "TS3Channels.h"

/*
 * ini file location: %APPDATA%\TS3Client
 */

#define CONF_FILE				"test_plugin" // ini file
#define CONF_APP				"TS3Client"   // application folder
#define CONF_OBJ(x)			QSettings x(QSettings::IniFormat, QSettings::UserScope, CONF_APP, CONF_FILE);

QStringList Config::getChannelTreeViewEntry(TS3Channels::ChannelInfo ch)
{
    QStringList strings;
    std::ostringstream convert;

    strings.append(QString(ch.description.c_str()));
    convert << ch.channelID;
    strings.append(QString(convert.str().c_str()));

    return strings;
}

void Config::addChannel(QTreeWidgetItem* parent, vector<TS3Channels::ChannelInfo>& ch, uint* index, int indent)
{
    QTreeWidgetItem* tree;

    while ((*index) < ch.size())
    {
        if (ch[*index].depth < indent)
        {
            break;
        }
        else if (ch[*index].depth == indent)
        {
            tree = new QTreeWidgetItem(parent, getChannelTreeViewEntry(ch[(*index)++]));
        }
        else if (ch[*index].depth > indent)
        {
            addChannel(tree, ch, index, ch[*index].depth);
        }
    }
}

void Config::addChannelList(QTreeWidget* parent, vector<TS3Channels::ChannelInfo>& ch, uint* index, int indent)
{
    QTreeWidgetItem* tree;

    // Empty out anything that was already there.
    parent->clear();

    // Set up the headers, but hide column 1 which is information for us, not the user...
    QTreeWidgetItem* headerItem = new QTreeWidgetItem();
    headerItem->setText(0, QStringLiteral("Room"));
    headerItem->setText(1, QStringLiteral("Channel"));
    parent->setHeaderItem(headerItem);
    parent->setHeaderHidden(false);
    parent->hideColumn(1);

    // Populate the tree view with the root entry, and any children if there are any
    tree = new QTreeWidgetItem(parent, getChannelTreeViewEntry(ch[(*index)++]));
    if ((*index < ch.size()))
        addChannel(tree, ch, index, ch[*index].depth);

    // Expand the root item to show its contents
    parent->expandItem(tree);
}


Config::Config(TS3Channels& tch)
{
    bool bl;
	setupUi(this);
	CONF_OBJ(cfg);

    chList = &tch;

    vector<TS3Channels::ChannelInfo> channels;

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

    uint i = 0;
    channels = tch.getChannelList();
    addChannelList(treeParentChannel, channels, &i, 0);

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

void Config::newRoot()
{
    vector<TS3Channels::ChannelInfo> channels;

    QList<QTreeWidgetItem*> t = treeParentChannel->selectedItems();
    QTreeWidgetItem* qtwi = t.at(0);
    QString channel = qtwi->text(1);
    int iChannel = channel.toInt();

    uint i = 0;
    channels = chList->getChannelList(iChannel);
    addChannelList(treeUntunedChannel, channels, &i, 0);

}