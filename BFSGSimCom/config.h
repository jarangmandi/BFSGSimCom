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

    Config(TS3Channels&);
    ~Config();
    int exec(void);

    ConfigMode getMode(void) { return mode; };
    bool getUntuned(void) { return blUntuned; };
    uint64 getRootChannel(void) { return iRoot; };
    uint64 getUntunedChannel(void) { return iUntuned; };

protected slots:
    void accept();
    void reject();
    void newRoot();
    void newUntuned();
    void columnResize(QTreeWidgetItem*);
    void modeChanged();
    void untunedChanged();

private:
    ConfigMode mode;
    bool blUntuned;
    uint64 iRoot;
    uint64 iUntuned;
    bool initialising = true;

    TS3Channels* chList = NULL;
    QStringList getChannelTreeViewEntry(TS3Channels::ChannelInfo ch);
    uint64 getSelectedChannelId(QTreeWidget* parent);
    void addChannelList(QTreeWidget* qtree, vector<TS3Channels::ChannelInfo>& channels, uint64 selection);
    QTreeWidgetItem* addChannel(QTreeWidgetItem* qtree, vector<TS3Channels::ChannelInfo>& channels, uint* index, int indent, uint64 selection);

};

#endif // CONFIG_H
