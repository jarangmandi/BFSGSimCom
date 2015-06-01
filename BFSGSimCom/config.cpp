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

// Utility function to generate the string list required to populate a QTreeViewWidget
QStringList Config::getChannelTreeViewEntry(TS3Channels::ChannelInfo ch)
{
    QStringList strings;
    std::ostringstream convert;

    strings.append(QString(ch.description.c_str()));
    convert << ch.channelID;
    strings.append(QString(convert.str().c_str()));

    return strings;
}

// A recursive function which adds the contents of a tree of channels to a root node which is passed in.
QTreeWidgetItem* Config::addChannel(QTreeWidgetItem* parent, vector<TS3Channels::ChannelInfo>& ch, uint* index, int indent, uint64 selection)
{
    QTreeWidgetItem* tree;
    QTreeWidgetItem* retValue = NULL;

    // For as long as we've got channels to add...
    while ((*index) < ch.size())
    {
        // If the node node is at a higher level than that which we're working with, then break out of the loop
        // and return back up the call chain to the previous heirarchical level.
        if (ch[*index].depth < indent)
        {
            break;
        }
        // If the current node is at the level at which we're working, then add it to the current parent and point
        // to the next node.
        else if (ch[*index].depth == indent)
        {
            tree = new QTreeWidgetItem(parent, getChannelTreeViewEntry(ch[*index]));
            if (ch[*index].channelID == selection)
                retValue = tree;
            (*index)++;
        }
        // If the current node is at a level deeper than that at which we're working, then drop down into
        // that level passing the most recently added node as the parent.
        else if (ch[*index].depth > indent)
        {
            QTreeWidgetItem* t = addChannel(tree, ch, index, ch[*index].depth, selection);
            if (t != NULL)
            {
                tree->setExpanded(true);
                retValue = t;
            }

        }
    }

    return retValue;
}

void Config::addChannelList(QTreeWidget* parent, vector<TS3Channels::ChannelInfo>& ch, uint64 selection)
{
    uint index = 0;
    QTreeWidgetItem* tree;
    QTreeWidgetItem* selectedItem = NULL;


    // Empty out anything that was already there.
    parent->clear();

    // Set up the headers, but hide column 1 which is information for us, not the user...
    QTreeWidgetItem* headerItem = new QTreeWidgetItem();
    headerItem->setText(0, QStringLiteral("Room"));
    headerItem->setText(1, QStringLiteral("Channel"));
    parent->setHeaderItem(headerItem);
    parent->setHeaderHidden(false);
#if !defined(_DEBUG)
    parent->hideColumn(1);
#endif

    // Populate the tree view with the root entry and expand it
    tree = new QTreeWidgetItem(parent, getChannelTreeViewEntry(ch[index++]));
    parent->expandItem(tree);

    // Add any children if there are any, and set the relevant item selected if we've found it.
    if ((index < ch.size()))
    {
        selectedItem = addChannel(tree, ch, &index, ch[index].depth, selection);
        if (selectedItem != NULL) selectedItem->setSelected(true);
    }

    if (selectedItem == NULL)
    {
        tree->setSelected(true);
    }

}

int Config::exec(void)
{
    vector<TS3Channels::ChannelInfo> channels;

    // Populate the root channel view...
    // As we do this, the untuned channel view should be automatically populated!
    channels = chList->getChannelList();
    addChannelList(treeParentChannel, channels, iRoot);
    treeParentChannel->resizeColumnToContents(0);
    treeParentChannel->resizeColumnToContents(1);

    return QDialog::exec();
}

Config::Config(TS3Channels& tch)
{
    bool blD;
    bool blM;
    bool blA;
    bool blU;
    setupUi(this);
	CONF_OBJ(cfg);

    chList = &tch;

    vector<TS3Channels::ChannelInfo> channels;

    // Restore selected channel IDs
    iRoot = cfg.value("channel/root", TS3Channels::CHANNEL_ID_NOT_FOUND).toULongLong();
    iUntuned = cfg.value("channel/untuned", TS3Channels::CHANNEL_ID_NOT_FOUND).toULongLong();

    //// Populate the root channel view...
    //// As we do this, the untuned channel view should be automatically populated!
    //channels = tch.getChannelList();
    //addChannelList(treeParentChannel, channels, iRoot);
    //treeParentChannel->resizeColumnToContents(0);
    //treeParentChannel->resizeColumnToContents(1);

    blU = cfg.value("untuned/move").toBool();
    rbUntunedMove->setChecked(blU);
    rbUntunedStay->setChecked(!blU);
    untunedChanged();

    // Set up the mode radio buttons and define the "mode" variable.
    blD = cfg.value("mode/disabled").toBool();
    rbDisabled->setChecked(blD);

    blM = cfg.value("mode/manual").toBool();
    rbEasyMode->setChecked(blM);

    blA = cfg.value("mode/auto").toBool();
    rbExpertMode->setChecked(blA);

    if (!(rbDisabled->isChecked() || rbEasyMode->isChecked() || rbExpertMode->isChecked()))
    {
        rbDisabled->setChecked(true);
    }

    mode = CONFIG_DISABLED;
    if (blM) mode = CONFIG_MANUAL;
    else if (blA) mode = CONFIG_AUTO;
    modeChanged();

    // No longer initialising...
    initialising = false;
}

Config::~Config()
{

}

// OK button pressed.
void Config::accept()
{
	CONF_OBJ(cfg);

    bool blD;
    bool blM;
    bool blA;
    bool blU;

    // Save the state of the operation mode buttons.
    blD = rbDisabled->isChecked();
    cfg.setValue("mode/disabled", blD);

    blM = rbEasyMode->isChecked();
    cfg.setValue("mode/manual", blM);

    blA = rbExpertMode->isChecked();
    cfg.setValue("mode/auto", blA);

    mode = CONFIG_DISABLED;
    if (blM) mode = CONFIG_MANUAL;
    else if (blA) mode = CONFIG_AUTO;

    // Save the state of the "untuned" buttons.
    blU = rbUntunedMove->isChecked();
    cfg.setValue("untuned/move", blU);

    // Save the channel IDs of each of the channel trees.
    iRoot = getSelectedChannelId(treeParentChannel);
    cfg.setValue("channel/root", iRoot);

    iUntuned = getSelectedChannelId(treeUntunedChannel);
    cfg.setValue("channel/untuned", iUntuned);
    
	QDialog::accept();
}

// Cancel button pressed...
void Config::reject()
{
	QDialog::reject();
}

// Returns the ID (from the second column) of the selected channel
uint64 Config::getSelectedChannelId(QTreeWidget* parent)
{
    uint64 iChannel = TS3Channels::CHANNEL_ID_NOT_FOUND;

    // Get the list of seleted items (there should be 0 or 1).
    QList<QTreeWidgetItem*> t = parent->selectedItems();
    if (t.size() > 0)
    {
        // Get the item, then the text, then an integer.
        QTreeWidgetItem* qtwi = t.at(0);
        QString channel = qtwi->text(1);
        iChannel = channel.toLongLong();
    }

    return iChannel;
}

// Invoked when a new channel is selected in the root channel tree.
void Config::newRoot()
{
    vector<TS3Channels::ChannelInfo> channels;

    uint64 iChannel;
    uint i = 0;

    // Get the root channel (which is the selected channel in the real root widget.
    iChannel = getSelectedChannelId(treeParentChannel);

    // If we've selected a new channel
    if (iChannel != TS3Channels::CHANNEL_ID_NOT_FOUND)
    {
        channels = chList->getChannelList(iChannel);
        addChannelList(treeUntunedChannel, channels, iUntuned);
        treeUntunedChannel->resizeColumnToContents(0);
        treeUntunedChannel->resizeColumnToContents(1);
    }
}

// Invoked when the selected "untuned" channel is changed.
void Config::newUntuned()
{
    iUntuned = getSelectedChannelId(treeUntunedChannel);
}

// Forces a resize of the column(s) that make up the tree so that everything is visible.
void Config::columnResize(QTreeWidgetItem* item)
{
    QTreeWidget* qtw = item->treeWidget();
    qtw->resizeColumnToContents(0);
    qtw->resizeColumnToContents(1);
}

// Invoked when the radio buttons that manage the operating mode.
void Config::modeChanged()
{
    bool bl = !(rbDisabled->isChecked());

    treeParentChannel->setEnabled(bl);
    gbUntuned->setEnabled(bl);
    
    untunedChanged();
}

// Invoked when the radio buttons that manage the action on an untuned channel are changed.
void Config::untunedChanged()
{
    bool bl = !(rbUntunedStay->isChecked());

    treeUntunedChannel->setEnabled(bl && gbUntuned->isEnabled());

}