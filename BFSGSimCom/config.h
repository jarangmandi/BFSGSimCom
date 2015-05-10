#ifndef CONFIG_H
#define CONFIG_H

//#include <QtGui/QMainWindow>
#include <QtWidgets/QDialog>
#include "ui_config.h"

class Config : public QDialog, public Ui::Config
{
    Q_OBJECT

public:
    Config();
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
};

#endif // CONFIG_H
