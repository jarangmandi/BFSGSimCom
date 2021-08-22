#ifndef CONFIG_H
#define CONFIG_H

//#include <QtGui/QMainWindow>
#include <QtWidgets/QDialog>
#include "ui_config.h"

#include <vector>
#include "TS3Channels.h"

using namespace std;

class Config : public QDialog, public Ui::Config
{
    Q_OBJECT

public:
    enum ConfigMode
    {
        CONFIG_DISABLED,
        CONFIG_MANUAL,
        CONFIG_AUTO
    };

    enum Spacing833Mode
    {
        SPACING_833_AUTO,
        SPACING_833_833,
        SPACING_833_25
    };

    Config(TS3Channels&);
    ~Config();
    int exec(void);

    ConfigMode getMode(void) { return mode; };
    void setMode(ConfigMode m);

    Spacing833Mode get833Mode(void) { return spacing833Mode; };

    bool getInfoDetailed(void) { return blInfoDetailed; };
    void setInfoDetailed(bool bl);
    bool getUntuned(void) { return blUntuned; };
    void setUntuned(bool bl);
    bool getOutOfRangeUntuned(void) { return blOutOfRangeUntuned; };
    bool getConsiderRange(void) { return blConsiderRange; };
    uint64 getRootChannel(void) { return iRoot; };
    uint64 getUntunedChannel(void) { return iUntuned; };
	string getRootChannelName(void) { return strRoot; };
	string getUntunedChannelName(void) { return strUntuned; };
	void populateChannelList(void);

protected slots:
    void accept();
    void reject();
    void newRoot();
    void newUntuned();
    void columnResize(QTreeWidgetItem*);
    void modeChanged();
    void untunedChanged();

private:
	// Structure for channel information
    ConfigMode mode;
    Spacing833Mode spacing833Mode;
    bool blInfoDetailed;
    bool blUntuned;
    bool blConsiderRange;
    bool blOutOfRangeUntuned;
	bool blRestartInManualMode;
    uint64 iRoot;
	string strRoot;
    uint64 iUntuned;
	string strUntuned;
    //bool initialising = true;

    TS3Channels* chList = NULL;
    QStringList getChannelTreeViewEntry(TS3Channels::ChannelInfo ch);
    tuple<uint64, string> getSelectedChannelId(QTreeWidget* parent);
    void addChannelList(QTreeWidget* qtree, vector<TS3Channels::ChannelInfo>& channels, uint64 selection);
    QTreeWidgetItem* addChannel(QTreeWidgetItem* qtree, vector<TS3Channels::ChannelInfo>& channels, uint* index, int indent, uint64 selection);
    void saveSettings(void);

};

#endif // CONFIG_H
