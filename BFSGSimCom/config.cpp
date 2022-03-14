#include <QtCore/QSettings>
#include <QtWidgets/QMessageBox>

#include <sstream>

#include "config.h"

#include "TS3Channels.h"

/*
 * ini file location: %APPDATA%\TS3Client
 */

#define CONF_ORG				"BFSG" // ini file
#define CONF_APP				"BFSGSimCom"   // application folder
//#define CONF_OBJ(x)			QSettings x(QSettings::IniFormat, QSettings::UserScope, CONF_APP, CONF_FILE);

// Utility function to generate the string list required to populate a QTreeViewWidget
QStringList Config::getChannelTreeViewEntry(TS3Channels::ChannelInfo ch)
{
    QStringList strings;
    std::ostringstream convert;

    strings.append(QString(ch.name.c_str()));
    convert << ch.channelID;
    strings.append(QString(convert.str().c_str()));

    return strings;
}

// A recursive function which adds the contents of a tree of channels to a root node which is passed in.
QTreeWidgetItem* Config::addChannel(QTreeWidgetItem* parent, vector<TS3Channels::ChannelInfo>& ch, uint* index, int indent, uint64 selection)
{
    QTreeWidgetItem* tree = NULL;
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
        // that level passing the most recently added node as the parent and getting back the node which
        // matches the selection we're looking for.
        else if (ch[*index].depth > indent)
        {
            QTreeWidgetItem* t = addChannel(tree, ch, index, ch[*index].depth, selection);
            if (t != NULL)
            {
                // if we have a match for the selection, then expand the parent node and save it for returning
                tree->setExpanded(true);
                retValue = t;
            }

        }
    }

    // return any node from this or lower levels that matches the one we're looking for.
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

void Config::populateChannelList(void)
{
	vector<TS3Channels::ChannelInfo> channels;

	// Populate the root channel view...
	// As we do this, the untuned channel view should be automatically populated!
	channels = chList->getChannelList();
	addChannelList(treeParentChannel, channels, iRoot);
	treeParentChannel->resizeColumnToContents(0);
	treeParentChannel->resizeColumnToContents(1);
}

int Config::exec(void)
{
	populateChannelList();
    return QDialog::exec();
}

void Config::setMode(ConfigMode mode)
{
    // Change the dialog selection based on the input value.
    switch (mode)
    {
    case ConfigMode::CONFIG_DISABLED:
        rbDisabled->setChecked(true);
        break;
    case ConfigMode::CONFIG_MANUAL:
        rbEasyMode->setChecked(true);
        break;
    case ConfigMode::CONFIG_AUTO:
        rbExpertMode->setChecked(true);
        break;
    default:
        break;
    }

    // Do whatever processing is required to make everything right.
    modeChanged();

    // As this function is only ever expected to be called by something which runs external to the GUI,
    // save the changes.
    saveSettings();
}

void Config::setUntuned(bool untuned)
{
    rbUntunedMove->setChecked(!untuned);
    rbUntunedStay->setChecked(untuned);

    untunedChanged();
    saveSettings();
}

void Config::setInfoDetailed(bool infoDetailed)
{
	cbInfoDetailed->setChecked(infoDetailed);
	blInfoDetailed = infoDetailed;
	saveSettings();
}

Config::Config(TS3Channels& tch)
{
    bool blD;
    bool blM;
    bool blA;

    bool bl833Automatic;
    bool bl833Force833;
    bool bl833Force25;

    bool blFreqModeMilitary;
    bool blFreqModeCivil;

    // The set default format line needs to be first because it defines the behaviour of the
    // QSettings constructor...
    QSettings settings(QSettings::Format::IniFormat, QSettings::Scope::UserScope, QString(CONF_ORG), QString(CONF_APP));

    chList = &tch;

    setupUi(this);

    // Restore the detailed information checkbox
    blInfoDetailed = settings.value("info/detailed").toBool();
    cbInfoDetailed->setChecked(blInfoDetailed);

    // Restore selected channel IDs
    iRoot = settings.value("channel/root", TS3Channels::CHANNEL_ID_NOT_FOUND).toULongLong();
    iUntuned = settings.value("channel/untuned", TS3Channels::CHANNEL_ID_NOT_FOUND).toULongLong();

    blUntuned = settings.value("untuned/move").toBool();
    blOutOfRangeUntuned = settings.value("untuned/range").toBool();
    rbUntunedMove->setChecked(blUntuned);
    rbUntunedStay->setChecked(!blUntuned);
    cbUntunedRange->setChecked(blOutOfRangeUntuned);
    
    untunedChanged();

	blRestartInManualMode = settings.value("mode/manualRestart").toBool();
	cbManualModeOnStart->setChecked(blRestartInManualMode);

    // Set up the mode radio buttons and define the "mode" variable.
    blD = settings.value("mode/disabled").toBool();
    rbDisabled->setChecked(blD);

    blM = settings.value("mode/manual").toBool() || blRestartInManualMode;
    rbEasyMode->setChecked(blM);

	blA = settings.value("mode/auto").toBool() && !blRestartInManualMode;
    rbExpertMode->setChecked(blA);

    blConsiderRange = settings.value("mode/considerRange").toBool();
    cbConsiderRange->setChecked(blConsiderRange);

    if (!(rbDisabled->isChecked() || rbEasyMode->isChecked() || rbExpertMode->isChecked()))
        rbDisabled->setChecked(true);

    if (blM) mode = ConfigMode::CONFIG_MANUAL;
    else if (blA) mode = ConfigMode::CONFIG_AUTO;
    else mode = ConfigMode::CONFIG_DISABLED;
    modeChanged();

    // Restore the 833 handling radio boxes
    bl833Force833 = settings.value("833/force833").toBool();
    bl833Force25 = settings.value("833/force25").toBool();
    bl833Automatic = settings.value("833/automatic").toBool();

    rb833Force25->setChecked(bl833Force25);
    rb833Force833->setChecked(bl833Force833);
    rb833Default->setChecked(bl833Automatic);

    if (!(rb833Default->isChecked() || rb833Force833->isChecked() || rb833Force25->isChecked()))
        rb833Default->setChecked(true);

    if (bl833Force833) spacing833Mode = Spacing833Mode::SPACING_833_833;
    else if (bl833Force25) spacing833Mode = Spacing833Mode::SPACING_833_25;
    else spacing833Mode = Spacing833Mode::SPACING_833_AUTO;

    // Restore the frequency mode radio boxes
    blFreqModeMilitary = settings.value("freqmode/military").toBool();
    blFreqModeCivil = settings.value("freqmode/civil").toBool();
    rbFreqModeCivil->setChecked(blFreqModeCivil);
    rbFreqModeMilitary->setChecked(blFreqModeMilitary);

    if (!(rbFreqModeCivil->isChecked() || rbFreqModeMilitary->isChecked()))
        rbFreqModeCivil->setChecked(true);

    if (blFreqModeMilitary) frequencyMode = FrequencyMode::FREQUENCY_MILITARY;
    else frequencyMode = FrequencyMode::FREQUENCY_CIVIL;
}

Config::~Config()
{

}

void Config::saveSettings()
{
	QSettings settings(QSettings::Format::IniFormat, QSettings::Scope::UserScope, QString(CONF_ORG), QString(CONF_APP));

	// Save the state of the detailed information pane option
	blInfoDetailed = cbInfoDetailed->isChecked();
	settings.setValue("info/detailed", blInfoDetailed);

    // Save the 833 handling information
    bool bl833Automatic = rb833Default->isChecked();
    bool bl833Force833 = rb833Force833->isChecked();
    bool bl833Force25 = rb833Force25->isChecked();
    settings.setValue("833/automatic", bl833Automatic);
    settings.setValue("833/force833", bl833Force833);
    settings.setValue("833/force25", bl833Force25);

    if (bl833Force833) spacing833Mode = Spacing833Mode::SPACING_833_833;
    else if (bl833Force25) spacing833Mode = Spacing833Mode::SPACING_833_25;
    else spacing833Mode = Spacing833Mode::SPACING_833_AUTO;

    // Save the Frequency mode
    bool blFreqModeMilitary = rbFreqModeMilitary->isChecked();
    bool blFreqModeCivil = rbFreqModeCivil->isChecked();
    settings.setValue("freqmode/military", blFreqModeMilitary);
    settings.setValue("freqmode/civil", blFreqModeCivil);

    if (blFreqModeMilitary) frequencyMode = FrequencyMode::FREQUENCY_MILITARY;
    else frequencyMode = FrequencyMode::FREQUENCY_CIVIL;

    // Save the state of the operation mode buttons.
    bool blD = rbDisabled->isChecked();
    bool blM = rbEasyMode->isChecked();
    bool blA = rbExpertMode->isChecked();

    settings.setValue("mode/disabled", blD);
    settings.setValue("mode/manual", blM);
    settings.setValue("mode/auto", blA);

	blRestartInManualMode = cbManualModeOnStart->isChecked();
	settings.setValue("mode/manualRestart", blRestartInManualMode);

    if (blM) mode = ConfigMode::CONFIG_MANUAL;
    else if (blA) mode = ConfigMode::CONFIG_AUTO;
    else mode = ConfigMode::CONFIG_DISABLED;

	// Save the state of the "Consider Range" checkbox
	blConsiderRange = cbConsiderRange->isChecked();
	settings.setValue("mode/considerRange", blConsiderRange);

	// Save the state of the "untuned" buttons.
    blUntuned = rbUntunedMove->isChecked();
    settings.setValue("untuned/move", blUntuned);

    blOutOfRangeUntuned = cbUntunedRange->isChecked();
    settings.setValue("untuned/range", blOutOfRangeUntuned);

    // Save the channel IDs of each of the channel trees.
    settings.setValue("channel/root", iRoot);
    settings.setValue("channel/untuned", iUntuned);

}

// OK button pressed.
void Config::accept()
{
    if (rbUntunedMove->isChecked() && iUntuned == 0)
    {
        QMessageBox qMsg;
        qMsg.setIcon(QMessageBox::Icon::Critical);
        qMsg.setStandardButtons(QMessageBox::StandardButton::Ok);
        qMsg.setText("Error in configuration");
        qMsg.setInformativeText("It is not possible to move to the \"Root\" channel when no frequency is tuned.\r\n\r\nEither select \"Stay in current channel\", or select a valid untuned channel.");
        qMsg.setDefaultButton(QMessageBox::StandardButton::Ok);
        qMsg.exec();
    }
    else
    {
        saveSettings();
        QDialog::accept();
    }
}

// Cancel button pressed...
void Config::reject()
{
	QDialog::reject();
}

// Returns the ID (from the second column) of the selected channel
tuple<uint64, string> Config::getSelectedChannelId(QTreeWidget* parent)
{
	uint64 iChannel = 0;
	string strChannel = "";

    // Get the list of seleted items (there should be 0 or 1).
    QList<QTreeWidgetItem*> t = parent->selectedItems();
    if (t.size() > 0)
    {
        // Get the item, then the text, then an integer.
        QTreeWidgetItem* qtwi = t.at(0);
        QString channelID = qtwi->text(1);
		QString channelName = qtwi->text(0);

        iChannel = channelID.toLongLong();
		strChannel = ::string(channelName.toUtf8().constData());
    }

    return ::make_tuple(iChannel, strChannel);
}

// Invoked when a new channel is selected in the root channel tree.
void Config::newRoot()
{
    vector<TS3Channels::ChannelInfo> channels;

    // Get the root channel (which is the selected channel in the real root widget.
	tuple<uint64, string> chInfo = getSelectedChannelId(treeParentChannel);
    ::tie(iRoot, strRoot) = chInfo;

    // If we've selected a new channel
    if (iRoot != TS3Channels::CHANNEL_ID_NOT_FOUND)
    {
        channels = chList->getChannelList(iRoot);
        addChannelList(treeUntunedChannel, channels, iUntuned);
        treeUntunedChannel->resizeColumnToContents(0);
        treeUntunedChannel->resizeColumnToContents(1);
    }
}

// Invoked when the selected "untuned" channel is changed.
void Config::newUntuned()
{
	tuple<uint64, string> chInfo = getSelectedChannelId(treeUntunedChannel);
	::tie(iUntuned, strUntuned) = chInfo;
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
    bool blNotDisabled = !(rbDisabled->isChecked());
	bool blAuto = rbExpertMode->isChecked();

    treeParentChannel->setEnabled(blNotDisabled);
    cbConsiderRange->setEnabled(blNotDisabled);
    gbUntuned->setEnabled(blNotDisabled);

	cbManualModeOnStart->setEnabled(blAuto);

    untunedChanged();
}

// Invoked when the radio buttons that manage the action on an untuned channel are changed.
void Config::untunedChanged()
{
    bool bl = !(rbUntunedStay->isChecked());

    treeUntunedChannel->setEnabled(bl && gbUntuned->isEnabled());
    cbUntunedRange->setEnabled(bl && cbConsiderRange->isChecked() && gbUntuned->isEnabled());
}
