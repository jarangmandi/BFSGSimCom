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
    Config(TS3Channels&);
    ~Config();

    enum ConfigMode
    {
        CONFIG_DISABLED,
        CONFIG_MANUAL,
        CONFIG_AUTO
    };

    ConfigMode mode;

protected slots:
    void accept();
    void reject();
    void newRoot();

private:
    TS3Channels* chList = NULL;
    QStringList getChannelTreeViewEntry(TS3Channels::ChannelInfo ch);
    void addChannelList(QTreeWidget* qtree, vector<TS3Channels::ChannelInfo>& channels, uint* index, int indent);
    void addChannel(QTreeWidgetItem* qtree, vector<TS3Channels::ChannelInfo>& channels, uint* index, int indent);
};

#endif // CONFIG_H
